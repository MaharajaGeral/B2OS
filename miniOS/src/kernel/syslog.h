#pragma once
#include "kernel/types.h"

// Log kernel untuk memperlihatkan fitur OS yang bekerja saat aplikasi dipakai.
// Perintah shell 'syslog on|off' memanggil syslog::enable().
// Semua fungsi log DIAM bila syslog mati, dan tidak memanggil kmalloc.
// Format keluaran:  "[TAG   ] pesan"   (TAG dipadatkan jadi 6 karakter:
//                   THREAD MUTEX SEM MEMORI VFS AKSES APP)
namespace syslog {
void enable(bool on);      // juga menyalakan/mematikan thread::set_trace
bool enabled();

void log(const char* tag, const char* msg);                    // "[TAG   ] msg"
void log_s(const char* tag, const char* msg, const char* text);// "[TAG   ] msg text"
void log_n(const char* tag, const char* msg, u32 n);           // "[TAG   ] msg n"

// Tambahan (tidak mengubah fungsi di atas): "[TAG   ] who msg text"
// contoh: log_who("MUTEX", "petugas-1", "mengunci", "inventaris")
void log_who(const char* tag, const char* who, const char* msg, const char* text);
}