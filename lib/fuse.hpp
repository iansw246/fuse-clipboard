#pragma once

#define FUSE_USE_VERSION 35
#include "clipboardData.hpp"
#include <fuse3/fuse.h>

namespace FuseImplementation
{
struct FuseInitData {
    ClipboardData* clipboardData;
};

extern const fuse_operations operations;

std::string fullMimeTypeToFileName(const std::string& mimeType);
std::string filePathToFullMimeType(const std::string& filePath);
}
