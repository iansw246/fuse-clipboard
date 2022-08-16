#include "fuse.hpp"

#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QSize>
#include <QBuffer>
#include <QByteArray>
#include <QDebug>

#include <cstring>
#include <errno.h>
#include <array>
#include <string_view>
#include <mutex>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

using namespace FuseImplementation;

/* TODO:
 * Use file handles or other caching to avoid checking file path every time.
 * Create interface for clipboard data to eventually add...
 * GTK or raw X11 or other mechanism for clipboard access so QT is not required
*/

// Fuse private data is same as init data, for now
struct FusePrivateData : FuseInitData
{};

// Should use std::string but it's not constexpr yet (as of C++17) and this is an unnecessary optimization I want to make
constexpr std::string_view CLIPBOARD_BASE_PATH("/clipboard");
constexpr std::string_view IMAGE_FILENAME("image");
constexpr std::string_view CLIPBOARD_IMAGE_BASE_PATH("/clipboard/image.");
// Basename of every file in FUSE filesystem, without extension.
constexpr std::string_view BASE_FILE_NAME("file");

// 0 flag for filler function. In most examples, 0 is passed for this flag. There isn't a enum for 0, so to pass a 0 in C++, need to cast 0 to the enum
// This is undefined because there isn't an enum for 0, but it should be ok.
constexpr fuse_fill_dir_flags FUSE_FILL_DIR_NO_FLAG = static_cast<fuse_fill_dir_flags>(0);

FusePrivateData* getPrivateData()
{
    auto* privateData = reinterpret_cast<FusePrivateData*>(fuse_get_context()->private_data);
    return privateData;
}
ClipboardData* getClipboardData()
{
    return getPrivateData()->clipboardData;
}

// Removes leading part of mime type, returns name of file as {BASE_FILE_NAME}.{mimetype}
// If mimetype is "image/png" and BASE_FILE_NAME="file", file name returned is "file.png"
std::string fullMimeTypeToFileName(const std::string& mimeType)
{
    auto slashIndex = mimeType.find('/');
    // Maybe reserve space to increase performance
    std::string fileName = BASE_FILE_NAME.data() + ("." + mimeType.substr(slashIndex + 1));
    return fileName;
}

// Given file path in form of "{mimetype prefix}/{name}.{mimetype}", returns full mimetype, {mimetype prefix}/{mimetype}
// For example, if base file name is "basefile", then "image/basefile.png" returns "image/png"
std::string filePathToFullMimeType(const std::string& filePath)
{
    auto baseFileNameIndex = filePath.find(BASE_FILE_NAME.data());
    if (baseFileNameIndex == std::string::npos)
    {
        return std::string();
    }
    auto dotIndex = filePath.find('.', baseFileNameIndex + BASE_FILE_NAME.size());
    if (dotIndex == std::string::npos)
    {
        return std::string();
    }
    // +1 to skip dot
    std::string mimeType = filePath.substr(0, baseFileNameIndex) + filePath.substr(dotIndex + 1);
    return mimeType;
}

void* init(fuse_conn_info* conn, fuse_config* config)
{
    qDebug() << "Private data pointer: " << fuse_get_context()->private_data << '\n';
    // Since init data is same as private data, no need to change anything
    return fuse_get_context()->private_data;
}

int getAttr(const char* path, struct stat* stbuf, fuse_file_info* fi)
{
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        // 1 subdir, /clipboard/
        stbuf->st_nlink = 3;
        return 0;
    }
    else if (strcmp(path, CLIPBOARD_BASE_PATH.data()) == 0)
    {
        ClipboardData* clipboardData = getClipboardData();
        auto lock = clipboardData->getLock();
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2 + clipboardData->mainMimeTypesCount();
        return 0;
    }
    // path starts with /clipboard
    else if (strncmp(path, CLIPBOARD_BASE_PATH.data(), CLIPBOARD_BASE_PATH.size()) == 0)
    {
        // Check if path is a mime dir

        // skip leading /clipboard/
        std::string pathWithoutClipboardType(path + CLIPBOARD_BASE_PATH.size() + 1);

        ClipboardData* clipboardData = getClipboardData();
        auto lock = clipboardData->getLock();

        // Check for main mime type directory
        // If pathWithoutClipboardType is a mimeprefix, like "image" or "text"
        if (clipboardData->hasMainMimeType(pathWithoutClipboardType))
        {
            stbuf->st_mode = S_IFDIR | 0755;
            // pathWIthoutClipboard has been verified to be a main MIME type
            stbuf->st_nlink = 2 + clipboardData->mimeSubTypesCount(pathWithoutClipboardType);
            return 0;
        }

        // Check if file, skipping "/clipboard/", matches a full mimetype we have data for
        std::string possibleFullMimeType = filePathToFullMimeType(pathWithoutClipboardType);
        std::optional<size_t> dataSize = clipboardData->dataSize(possibleFullMimeType);
        if (dataSize)
        {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = *dataSize;
            return 0;
        }
    }

    return -ENOENT;
}

int readDir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info* fi,
            fuse_readdir_flags flags)
{
    //filler(buf, ".", NULL, 0, fuse_fill_dir_flags::FUSE_FILL_DIR_PLUS);
    filler(buf, ".", NULL, 0, FUSE_FILL_DIR_NO_FLAG);
    filler(buf, "..", NULL, 0, FUSE_FILL_DIR_NO_FLAG);
    if (strcmp(path, "/") == 0)
    {
        filler(buf, "clipboard", NULL, 0, FUSE_FILL_DIR_NO_FLAG);
        return 0;
    }
    else if (strcmp(path, "/clipboard") == 0)
    {
        ClipboardData* clipboardData = getClipboardData();
        auto lock = clipboardData->getLock();
        const auto mainMimeTypes = clipboardData->mainMimeTypes();
        for (const std::string& mainMimeType : mainMimeTypes)
        {
            filler(buf, mainMimeType.c_str(), NULL, 0, FUSE_FILL_DIR_NO_FLAG);
        }
        return 0;
    }
    // If path start swith clipboard base path
    else if (strncmp(path, CLIPBOARD_BASE_PATH.data(), CLIPBOARD_BASE_PATH.size()) == 0)
    {
        ClipboardData* clipboardData = getClipboardData();
        auto lock = clipboardData->getLock();
        // Skip leading /clipboard/
        // +1 to skip last /
        // If path ends with main mime type, add subtypes to directory listing
        const auto subTypes = clipboardData->mimeSubTypes(path + CLIPBOARD_BASE_PATH.size() + 1);
        if (subTypes.empty())
        {
            return -ENOENT;
        }
        std::string filename(BASE_FILE_NAME.data());
        filename += ".";
        auto extensionIndex = filename.size();
        for (const std::string& subType : subTypes)
        {
            // Replace old extension with new extension
            filename.replace(extensionIndex, std::string::npos, subType);
            filler(buf, filename.c_str(), NULL, 0, FUSE_FILL_DIR_NO_FLAG);
        }
        return 0;
    }
    return -ENOENT;
}

int open(const char* path, fuse_file_info* fi)
{
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
    {
        return -EACCES;
    }
    if (strncmp(path, CLIPBOARD_BASE_PATH.data(), CLIPBOARD_BASE_PATH.size()) == 0)
    {
        ClipboardData* clipboardData = getClipboardData();
        auto lock = clipboardData->getLock();
        if (clipboardData->hasFullMimeType(filePathToFullMimeType(path + CLIPBOARD_BASE_PATH.size() + 1)))
        {
            return 0;
        }
    }
    return -ENOENT;
}

int read(const char* path, char* buf, size_t size, off_t offset, fuse_file_info* fi)
{
    if (strncmp(path, CLIPBOARD_BASE_PATH.data(), CLIPBOARD_BASE_PATH.size()) == 0)
    {
        ClipboardData* clipboardData = getClipboardData();
        auto lock = clipboardData->getLock();
        const std::optional<const std::vector<uint8_t>> data = clipboardData->mimeData(filePathToFullMimeType(path + CLIPBOARD_BASE_PATH.size() + 1));
        if (data)
        {
            if (size + offset >= data->size())
            {
                size = data->size() - offset;
            }
            std::copy_n(data->begin() + offset, size, buf);
            return size;
        }
    }
    return -ENOENT;
}

const fuse_operations FuseImplementation::operations = {.getattr = getAttr, .open = open, .read = read, .readdir = readDir, .init = init};
