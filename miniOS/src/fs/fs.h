#pragma once

#include "kernel/types.h"

const int FS_NAME_MAX  = 32;
const int FS_MAX_FILES = 64;

enum FsErr {
    FS_OK = 0,
    FS_ENOENT = -1,
    FS_EEXIST = -2,
    FS_ENOMEM = -3,
    FS_EPERM = -4,
    FS_EFULL = -5,
    FS_EINVAL = -6
};

const u8 FS_ADMIN_ONLY = 0x1;

struct FsFile {
    bool used;
    char name[FS_NAME_MAX];
    u8 owner;
    u8 flags;
    char* data;
    u32 size;
};

namespace fs {
    void init();

    int create(const char* name, const char* content);
    int read(const char* name, char* out, u32 out_cap);
    int write(const char* name, const char* content);
    int remove(const char* name);

    int count();
    const FsFile* at(int index);
    const FsFile* find(const char* name);
}

void demo_fs();
