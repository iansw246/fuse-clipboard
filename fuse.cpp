#include <cstring>
#include <errno.h>
#include <array>
#include <string_view>
#include <mutex>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

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

#include "fuse.hpp"

namespace FuseImplementation
{

struct ImageFormat
{
    const char* format;
    const char* fileExtension;
};
struct FusePrivateData
{
    const QClipboard* clipboard;
    std::mutex clipboardMutex;
};

// List of image formats that will be available in filesystem
// Right now, uses QT to convert to these image formats
constexpr std::array<ImageFormat, 3> SUPPORTED_IMAGE_FORMATS {{ {"BMP", "bmp"}, {"JPG", "jpg"}, {"PNG", "png"} }};

constexpr std::string_view CLIPBOARD_BASE_PATH("/clipboard");
constexpr std::string_view IMAGE_FILENAME = "image";
constexpr std::string_view CLIPBOARD_IMAGE_BASE_PATH("/clipboard/image.");

// 0 flag for filler function. In most examples, 0 is passed for this flag. There isn't a enum for 0, so to pass a 0 in C++, need to cast 0 to the enum
// This is undefined because there isn't an enum for 0, but it should be ok.
constexpr fuse_fill_dir_flags FUSE_FILL_DIR_NO_FLAG = static_cast<fuse_fill_dir_flags>(0);

std::mutex clipboardMutex;

std::mutex clipboardDataMapMutex;
std::unordered_map<std::string, std::vector<uint8_t>> clipboardDataMap;
// List of mime type prefixes (image, text, application, etc.)
std::unordered_set<std::string> clipboardMimeDirs;

FusePrivateData* getPrivateData()
{
    return reinterpret_cast<FusePrivateData*>(fuse_get_context()->private_data);
}

void* init(fuse_conn_info* conn, fuse_config* config)
{
    return nullptr;
}

int getAttr(const char* path, struct stat* stbuf, fuse_file_info* fi)
{
    std::lock_guard lock(clipboardDataMapMutex);
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        // 1 subdir, /clipboard/
        stbuf->st_nlink = 3;
        return 0;
    }
    else if (strcmp(path, CLIPBOARD_BASE_PATH.data()) == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2 + clipboardMimeDirs.size();
        return 0;
    }
    // path starts with /clipboard
    else if (strncmp(path, CLIPBOARD_BASE_PATH.data(), CLIPBOARD_BASE_PATH.size()) == 0)
    {
        // Check if path is a mime dir
        // skip leading /clipboard/
        if (clipboardMimeDirs.find(path + CLIPBOARD_BASE_PATH.size() + 1) != clipboardMimeDirs.cend())
        {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
        else
        {
            // Check if file, skipping "/clipboard/", matches a mimetype we have data for
            auto it = clipboardDataMap.find(path + CLIPBOARD_BASE_PATH.size() + 1);
            if (it != clipboardDataMap.cend())
            {
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;
                stbuf->st_size = (*it).second.size();
                return 0;
            }
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
        std::lock_guard lock(clipboardDataMapMutex);
        for (const std::string& folder : clipboardMimeDirs)
        {
            filler(buf, folder.c_str(), NULL, 0, FUSE_FILL_DIR_NO_FLAG);
        }
        return 0;
    }
    else if (strncmp(path, CLIPBOARD_BASE_PATH.data(), CLIPBOARD_BASE_PATH.size()) == 0)
    {
        std::lock_guard lock(clipboardDataMapMutex);
        for (const std::string& dir : clipboardMimeDirs)
        {
            // Skip leading /clipboard/
            // +1 to skip last /
            if ((path + CLIPBOARD_BASE_PATH.size() + 1) == dir)
            {
                for (const auto& mimePair : clipboardDataMap)
                {
                    // Check if mime type starts the name of the directory
                    // for example, if "image/png" starts with "image"
                    if (mimePair.first.rfind(dir, 0) != std::string::npos)
                    {
                        // Add listing for file with text after first slash
                        // If mimePair.second == "image/png", then add "png"
                        // +1 to skip slash
                        filler(buf, mimePair.first.c_str() + dir.size() + 1, NULL, 0, FUSE_FILL_DIR_NO_FLAG);
                    }
                }
                return 0;
            }
        }
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
        std::lock_guard lock(clipboardDataMapMutex);
        auto it = clipboardDataMap.find(path + CLIPBOARD_BASE_PATH.size() + 1);
        if (it != clipboardDataMap.cend())
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
        std::lock_guard lock(clipboardDataMapMutex);
        auto it = clipboardDataMap.find(path + CLIPBOARD_BASE_PATH.size() + 1);
        if (it != clipboardDataMap.cend())
        {
            const std::vector<uint8_t>& data = (*it).second;
            if (size + offset >= data.size())
            {
                size = data.size() - offset;
            }
            std::copy_n(data.begin() + offset, size, buf);
            return size;
        }
    }
    return -ENOENT;
}

void destroy(void* privateData)
{
//    fuse_context* context = fuse_get_context();
//    delete reinterpret_cast<FusePrivateData*>(context->private_data);
}

} // namespace FuseImplementation
