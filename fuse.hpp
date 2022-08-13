#pragma once

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <QClipboard>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace FuseImplementation
{
extern std::mutex clipboardDataMapMutex;
extern std::unordered_map<std::string, std::vector<uint8_t>> clipboardDataMap;
extern std::unordered_set<std::string> clipboardMimeDirs;

extern const fuse_operations operations;
}
