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

void* init(fuse_conn_info* conn, fuse_config* config);
int getAttr(const char* path, struct stat* stbuf, fuse_file_info* fi);
int open(const char* path, fuse_file_info* fi);
int read(const char* path, char* buf, size_t size, off_t offset, fuse_file_info* fi);
int readDir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info* fi,
            fuse_readdir_flags flags);
void destroy(void* privateData);
const fuse_operations operations = {.getattr = getAttr, .open = open, .read = read, .readdir = readDir, .init = init};
}
