#pragma once
#include "kernel/types.h"

// Kontrol hak akses sederhana (kriteria 8): dua pengguna, admin dan user.
enum class Op : u8 { READ, CREATE, WRITE, DELETE };

namespace access {
const u8 UID_ADMIN = 0;
const u8 UID_USER  = 1;

void        init();                                        // sesi awal = user (hak paling kecil)
u8          current_uid();
const char* current_name();                                // "admin" atau "user"
bool        login(const char* name, const char* password); // false bila nama/kata sandi salah
void        logout();                                      // kembali menjadi user

// Kebijakan akses. file_flags bit 0x1 = admin-only (sama dengan FS_ADMIN_ONLY di fs/fs.h).
bool        allowed(Op op, u8 file_owner, u8 file_flags);
}

void demo_access();    // demo kriteria 8: akses sah berhasil, akses terlarang ditolak