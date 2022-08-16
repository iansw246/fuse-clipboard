#pragma once

#define FUSE_USE_VERSION 35
#include "clipboardData.hpp"
#include <fuse.h>

#include <memory>

namespace FuseImplementation
{
struct FuseInitData {
    ClipboardData* clipboardData;
};

extern const fuse_operations operations;
}
