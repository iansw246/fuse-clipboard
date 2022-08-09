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

constexpr std::string_view IMAGE_FILENAME = "image";
constexpr std::string_view CLIPBOARD_IMAGE_BASE_PATH("/clipboard/image.");

// 0 flag for filler function. In most examples, 0 is passed for this flag. There isn't a enum for 0, so to pass a 0 in C++, need to cast 0 to the enum
// This is undefined because there isn't an enum for 0, but it should be ok.
constexpr fuse_fill_dir_flags FUSE_FILL_DIR_NO_FLAG = static_cast<fuse_fill_dir_flags>(0);

std::mutex clipboardMutex;

std::mutex clipboardDataMapMutex;
std::unordered_map<std::string, std::vector<uint8_t>> clipboardDataMap;
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
    std::lock_guard lock(clipboardMutex);
    const QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* clipboardMimeData = clipboard->mimeData();
    {
        const QStringList formats = clipboardMimeData->formats();
        qDebug() << "Supported formats: ";
        for (auto it = formats.constBegin(); it != formats.constEnd(); ++it)
        {
            qDebug() << (*it).toLocal8Bit().constData();
        }
    }
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        // Subdirectory /clipboard/
        stbuf->st_nlink = 3;
    }
    else if (strcmp(path, "/clipboard") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    // If path starts with the image base path
    else if (clipboardMimeData->hasImage() && strncmp(path, CLIPBOARD_IMAGE_BASE_PATH.data(), CLIPBOARD_IMAGE_BASE_PATH.size()) == 0)
    {
        if (strcmp(path + CLIPBOARD_IMAGE_BASE_PATH.size(), "png") == 0 && clipboardMimeData->hasFormat("image/png"))
        {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = clipboardMimeData->data("image/png").size();
            return 0;
        }
        else if (strcmp(path + CLIPBOARD_IMAGE_BASE_PATH.size(), "jpg") == 0 && clipboardMimeData->hasFormat("image/jpeg"))
        {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = clipboardMimeData->data("image/jpeg").size();
            return 0;
        }
        else if (strcmp(path + CLIPBOARD_IMAGE_BASE_PATH.size(), "bmp") == 0 && clipboardMimeData->hasFormat("image/bmp"))
        {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = clipboardMimeData->data("image/bmp").size();
            return 0;
        }
        /*
        for (const ImageFormat& imageFormat : SUPPORTED_IMAGE_FORMATS)
        {
            const char* extension = path + CLIPBOARD_IMAGE_BASE_PATH.size();
            if (strcmp(extension, imageFormat.fileExtension) == 0)
            {
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;

                QBuffer buffer;
                const QImage image = clipboard->image(QClipboard::Mode::Clipboard);
                buffer.open(QIODevice::WriteOnly);
                image.save(&buffer, imageFormat.format);
                stbuf->st_size = buffer.buffer().size();
                return 0;
            }
        }
        */
    }
    else
    {
        return -ENOENT;
    }
    return 0;
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
    else if (strcmp(path, "/clipboard/"))
    {
        std::lock_guard lock(clipboardDataMapMutex);
        for (const std::string& folder : clipboardMimeDirs)
        {
            filler(buf, folder.c_str(), NULL, 0, FUSE_FILL_DIR_NO_FLAG);
        }
        /*std::lock_guard lock(clipboardMutex);
        const QMimeData* mimeData = QGuiApplication::clipboard()->mimeData();
        if (mimeData->hasImage())
        {
            for (const ImageFormat& imageFormat : SUPPORTED_IMAGE_FORMATS)
            {
                // Add image files with extension to directory list
                // For example, image.png, image.jpg, image.bmp
                std::string filename = std::string(IMAGE_FILENAME) + "." + imageFormat.fileExtension;
                filler(buf, filename.c_str(), NULL, 0, fuse_fill_dir_flags::FUSE_FILL_DIR_PLUS);
            }
        }*/
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
    // Path without extension is an image
    if (strncmp(path, CLIPBOARD_IMAGE_BASE_PATH.data(), CLIPBOARD_IMAGE_BASE_PATH.size()) == 0)
    {
        std::lock_guard lock(clipboardMutex);
        const QClipboard* clipboard = QGuiApplication::clipboard();
        const QMimeData* mimeData = clipboard->mimeData();
        if (mimeData->hasImage())
        {
            for (const ImageFormat& format : SUPPORTED_IMAGE_FORMATS)
            {
                if (strcmp(path + CLIPBOARD_IMAGE_BASE_PATH.size(), format.fileExtension) == 0)
                {
                    return 0;
                }
            }

        }
    }
    return -ENOENT;
}

int read(const char* path, char* buf, size_t size, off_t offset, fuse_file_info* fi)
{
    if (strncmp(path, CLIPBOARD_IMAGE_BASE_PATH.data(), CLIPBOARD_IMAGE_BASE_PATH.size()) == 0)
    {
        std::lock_guard lock(clipboardMutex);
        const QClipboard* clipboard = QGuiApplication::clipboard();
        const QMimeData* mimeData = clipboard->mimeData();
        if (mimeData->hasImage())
        {
            for (const ImageFormat& format : SUPPORTED_IMAGE_FORMATS)
            {
                if (strcmp(path + CLIPBOARD_IMAGE_BASE_PATH.size(), format.fileExtension) == 0)
                {
                    const QImage image = clipboard->image();
                    QBuffer buffer;
                    buffer.open(QIODevice::WriteOnly);
                    image.save(&buffer, format.format);
                    buffer.close();
                    const QByteArray& imageData = buffer.data();
                    if (offset >= imageData.size())
                    {
                        return 0;
                    }
                    if (offset + size > imageData.size())
                    {
                        size = imageData.size() - offset;
                    }
                    auto start = imageData.cbegin() + offset;
                    std::copy_n(start, size, buf);
                    return size;
                }
            }
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
