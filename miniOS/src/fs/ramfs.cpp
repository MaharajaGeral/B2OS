#include "monitor/monitor.h"
// ramfs.cpp - Filesystem sederhana berbasis memori (RAMFS)
#include "fs/fs.h"
#include "mem/kmalloc.h"
#include "kernel/cpu.h"
#include "kernel/kernel.h"
#include "security/access.h"

static FsFile files[FS_MAX_FILES];
static bool initialized = false;

// Menghitung panjang string
static u32 str_len(const char* s) {
    if (!s) return 0;

    u32 n = 0;
    while (s[n] != '\0') n++;
    return n;
}

// Menyalin string termasuk karakter null terminator
static void str_copy(char* dst, const char* src, u32 cap) {
    if (!dst || !src || cap == 0) return;

    u32 i = 0;
    while (i + 1 < cap && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

// Memastikan nama file valid
static bool valid_name(const char* name) {
    if (!name || name[0] == '\0') return false;

    u32 len = str_len(name);
    if (len >= FS_NAME_MAX) return false;

    // RAMFS versi awal hanya menggunakan nama file,
    // belum mendukung direktori atau path.
    for (u32 i = 0; i < len; i++) {
        if (name[i] == '/') return false;
    }

    return true;
}

// Mencari slot file berdasarkan nama
static int find_index(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && 
            __builtin_strcmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Mencari slot kosong
static int empty_index() {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!files[i].used) return i;
    }
    return -1;
}

namespace fs {

void init() {
    u32 f = irq_save();

    // Bebaskan data lama jika init dipanggil ulang
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && files[i].data) {
            kfree(files[i].data);
        }

        files[i].used = false;
        files[i].name[0] = '\0';
        files[i].owner = 0;
        files[i].flags = 0;
        files[i].data = nullptr;
        files[i].size = 0;
    }

    initialized = true;
    irq_restore(f);
}

// Membuat file baru
int create(const char* name, const char* content) {
    monitor::fs_event("create", name);
    if (!access::allowed(Op::CREATE, access::current_uid(), 0)) {
        { monitor::access_denied(); return FS_EPERM; }
    }
    if (!valid_name(name)) return FS_EINVAL;
    if (!initialized) init();

    if (!content) content = "";

    u32 size = str_len(content);

    u32 f = irq_save();

    if (find_index(name) >= 0) {
        irq_restore(f);
        return FS_EEXIST;
    }

    int slot = empty_index();
    if (slot < 0) {
        irq_restore(f);
        return FS_EFULL;
    }

    char* data = (char*)kmalloc(size + 1);
    if (!data) {
        irq_restore(f);
        return FS_ENOMEM;
    }

    str_copy(data, content, size + 1);

    files[slot].used = true;
    str_copy(files[slot].name, name, FS_NAME_MAX);
    files[slot].owner = access::current_uid();
    files[slot].flags = 0;
    files[slot].data = data;
    files[slot].size = size;

    irq_restore(f);
    return FS_OK;
}

// Membaca isi file
int read(const char* name, char* out, u32 out_cap) {
    monitor::fs_event("read", name);
    if (!valid_name(name) || !out || out_cap == 0)
        return FS_EINVAL;

    if (!initialized) init();

    u32 f = irq_save();

    int idx = find_index(name);
    if (idx < 0) {
        irq_restore(f);
        return FS_ENOENT;
    }

    FsFile* file = &files[idx];
    if (!access::allowed(Op::READ, file->owner, file->flags)) {
    irq_restore(f);
    { monitor::access_denied(); return FS_EPERM; }
}

    // Buffer harus cukup untuk data dan null terminator
    if (out_cap <= file->size) {
        irq_restore(f);
        return FS_EFULL;
    }

    str_copy(out, file->data, out_cap);

    int result = (int)file->size;
    irq_restore(f);
    return result;
}

// Mengganti isi file
int write(const char* name, const char* content) {
    monitor::fs_event("write", name);
    if (!valid_name(name)) return FS_EINVAL;
    if (!initialized) init();
    if (!content) content = "";

    u32 new_size = str_len(content);

    // Alokasi data baru terlebih dahulu
    char* new_data = (char*)kmalloc(new_size + 1);
    if (!new_data) return FS_ENOMEM;

    str_copy(new_data, content, new_size + 1);

    u32 f = irq_save();

    int idx = find_index(name);
    if (idx < 0) {
        irq_restore(f);
        kfree(new_data);
        return FS_ENOENT;
    }

    if (!access::allowed(Op::WRITE, files[idx].owner, files[idx].flags)) {
    irq_restore(f);
    kfree(new_data);
    { monitor::access_denied(); return FS_EPERM; }
    }

    // Ganti data lama dengan data baru
    char* old_data = files[idx].data;

    files[idx].data = new_data;
    files[idx].size = new_size;

    kfree(old_data);

    irq_restore(f);
    return FS_OK;
}

// Menghapus file
int remove(const char* name) {
    monitor::fs_event("remove", name);
    if (!valid_name(name)) return FS_EINVAL;
    if (!initialized) init();

    u32 f = irq_save();

    int idx = find_index(name);
    if (idx < 0) {
        irq_restore(f);
        return FS_ENOENT;
    }

    if (!access::allowed(Op::DELETE, files[idx].owner, files[idx].flags)) {
    irq_restore(f);
    { monitor::access_denied(); return FS_EPERM; }
    }

    if (files[idx].data) {
        kfree(files[idx].data);
    }

    files[idx].used = false;
    files[idx].name[0] = '\0';
    files[idx].owner = 0;
    files[idx].flags = 0;
    files[idx].data = nullptr;
    files[idx].size = 0;

    irq_restore(f);
    return FS_OK;
}

// Menghitung jumlah file aktif
int count() {
    if (!initialized) init();

    u32 f = irq_save();

    int n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used) n++;
    }

    irq_restore(f);
    return n;
}

// Mengambil file berdasarkan index slot
const FsFile* at(int index) {
    if (!initialized) init();
    if (index < 0 || index >= FS_MAX_FILES) return nullptr;

    u32 f = irq_save();
    const FsFile* result = files[index].used ? &files[index] : nullptr;
    irq_restore(f);

    return result;
}

// Mencari file berdasarkan nama
const FsFile* find(const char* name) {
    if (!valid_name(name)) return nullptr;
    if (!initialized) init();

    u32 f = irq_save();

    int idx = find_index(name);
    const FsFile* result = (idx >= 0) ? &files[idx] : nullptr;

    irq_restore(f);
    return result;
}

} // namespace fs

// Demo untuk melihat file yang tersimpan di RAMFS
void demo_fs() {
    kernel_print("\n--- RAMFS ---\n");

    int total = fs::count();
    kernel_print("Jumlah file: ");
    kernel_print_dec((u32)total);
    kernel_print("\n");

    for (int i = 0; i < FS_MAX_FILES; i++) {
        const FsFile* file = fs::at(i);
        if (!file) continue;

        kernel_print("File: ");
        kernel_print(file->name);
        kernel_print(" | Size: ");
        kernel_print_dec(file->size);
        kernel_print(" byte\n");
    }

    kernel_print("--- RAMFS selesai ---\n");
}