// Hak akses sederhana: pengguna "admin" dan "user" + kebijakan izin per operasi.
//
//   admin : boleh semua operasi pada semua file
//   user  : READ   boleh (kecuali file admin-only)
//           CREATE boleh
//           WRITE  hanya pada file miliknya sendiri
//           DELETE ditolak
//   file bertanda admin-only: ditolak untuk user pada semua operasi
#include "security/access.h"
#include "kernel/syslog.h"
#include "kernel/kernel.h"

static const u8 ADMIN_ONLY = 0x1;                 // sama dengan FS_ADMIN_ONLY di fs/fs.h
static const char* ADMIN_PASS = "admin123";       // kata sandi sederhana (cukup untuk tugas)
static const char* USER_PASS  = "user123";

static u8 cur_uid = access::UID_USER;             // awal: user

static bool same(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

void access::init() { cur_uid = UID_USER; }
u8   access::current_uid() { return cur_uid; }
const char* access::current_name() { return cur_uid == UID_ADMIN ? "admin" : "user"; }

bool access::login(const char* name, const char* password) {
    if (!name || !password) return false;
    if (same(name, "admin") && same(password, ADMIN_PASS)) {
        cur_uid = UID_ADMIN;
        syslog::log_s("AKSES", "login berhasil:", "admin");
        return true;
    }
    if (same(name, "user") && same(password, USER_PASS)) {
        cur_uid = UID_USER;
        syslog::log_s("AKSES", "login berhasil:", "user");
        return true;
    }
    syslog::log_s("AKSES", "login GAGAL untuk", name);
    return false;
}

void access::logout() {
    cur_uid = UID_USER;
    syslog::log("AKSES", "logout, kembali menjadi user");
}

bool access::allowed(Op op, u8 file_owner, u8 file_flags) {
    if (cur_uid == UID_ADMIN) return true;
    if (file_flags & ADMIN_ONLY) return false;

    switch (op) {
    case Op::READ: return true;
    case Op::CREATE: return false;
    case Op::WRITE: return false;
    case Op::DELETE: return false;
    }

return false;
}

// ---------- demo kriteria 8 ----------
static void row(const char* label, bool ok) {
    kernel_print("  ");
    kernel_print(label);
    int n = 0;
    while (label[n]) n++;
    while (n++ < 34) kernel_print(" ");
    kernel_print(ok ? "-> DIIZINKAN\n" : "-> DITOLAK\n");
}

static void matrix() {
    row("baca file milik admin",           access::allowed(Op::READ,   access::UID_ADMIN, 0));
    row("tambah file baru",                access::allowed(Op::CREATE, access::UID_USER,  0));
    row("ubah file milik sendiri",         access::allowed(Op::WRITE,  access::UID_USER,  0));
    row("ubah file milik admin",           access::allowed(Op::WRITE,  access::UID_ADMIN, 0));
    row("hapus file",                      access::allowed(Op::DELETE, access::UID_USER,  0));
    row("baca file khusus admin",          access::allowed(Op::READ,   access::UID_ADMIN, ADMIN_ONLY));
}

void demo_access() {
    u8 saved = cur_uid;                                    // pulihkan sesi pengguna di akhir
    kernel_print("--- demo_access: hak akses admin dan user ---\n");

    cur_uid = access::UID_USER;
    kernel_print("Pengguna aktif: user\n");
    matrix();

    kernel_print("Login admin dengan kata sandi SALAH: ");
    kernel_print(access::login("admin", "salah") ? "berhasil (seharusnya tidak!)\n" : "DITOLAK\n");

    kernel_print("Login admin dengan kata sandi benar: ");
    kernel_print(access::login("admin", ADMIN_PASS) ? "berhasil\n" : "gagal\n");
    kernel_print("Pengguna aktif: admin\n");
    matrix();

    cur_uid = saved;
    kernel_print("--- demo_access selesai, sesi dikembalikan ke ");
    kernel_print(access::current_name());
    kernel_print(" ---\n");
}