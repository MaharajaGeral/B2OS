#include "app/gudang.h"
#include "fs/fs.h"
#include "kernel/kernel.h"
#include "kernel/syslog.h"
#include "lib/string.h"
#include "proc/scheduler.h"
#include "sync/mutex.h"

namespace {

const u32  ISI_MAX = 64;                    // ukuran buffer isi berkas
const u32  RAK_MAX = 16;                    // rak maks 15 karakter + '\0'
const char LOG_FILE[] = "log_aktivitas";    // berkas pencatat, bukan barang

bool sudah_init = false;

// ---------- Helper teks (tanpa libc) ----------
u32 panjang(const char* s) {
    u32 n = 0;
    while (s[n]) n++;
    return n;
}

bool sama(const char* a, const char* b) {
    u32 i = 0;
    while (a[i] && a[i] == b[i]) i++;
    return a[i] == b[i];
}

// Salin src ke dst (kapasitas cap), selalu diakhiri '\0'. false bila terpotong.
bool salin(char* dst, u32 cap, const char* src) {
    u32 i = 0;
    while (src[i] && i + 1 < cap) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
    return src[i] == '\0';
}

// true bila 'teks' memuat 'kata' (kata kosong = cocok semua)
bool memuat(const char* teks, const char* kata) {
    u32 n = panjang(teks), k = panjang(kata);
    if (k == 0) return true;
    if (k > n) return false;
    for (u32 i = 0; i + k <= n; i++) {
        u32 j = 0;
        while (j < k && teks[i + j] == kata[j]) j++;
        if (j == k) return true;
    }
    return false;
}

// Angka -> teks desimal. 'out' minimal 11 byte.
void angka_ke_teks(u32 v, char* out) {
    char t[10];
    u32 n = 0;
    if (v == 0) t[n++] = '0';
    while (v) { t[n++] = (char)('0' + v % 10); v /= 10; }
    for (u32 i = 0; i < n; i++) out[i] = t[n - 1 - i];
    out[n] = '\0';
}

// ---------- Validasi masukan ----------
bool nama_valid(const char* nama) {
    u32 n = panjang(nama);
    if (n == 0 || n >= (u32)FS_NAME_MAX) return false;
    for (u32 i = 0; i < n; i++) {
        char c = nama[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
    }
    return true;
}

bool rak_valid(const char* rak) {
    u32 n = panjang(rak);
    if (n == 0 || n >= RAK_MAX) return false;
    for (u32 i = 0; i < n; i++) {
        if (rak[i] == ';' || rak[i] <= ' ') return false;   // tanpa spasi/kontrol/titik koma
    }
    return true;
}

// ---------- Model data: bentuk & urai isi berkas ----------
// Membentuk "stok=<angka>;rak=<teks>" ke 'out'. false bila tidak muat.
bool bentuk_isi(u32 stok, const char* rak, char* out, u32 cap) {
    char angka[11];
    angka_ke_teks(stok, angka);
    const char* bagian[4] = { "stok=", angka, ";rak=", rak };
    u32 n = 0;
    for (int b = 0; b < 4; b++) {
        for (u32 i = 0; bagian[b][i]; i++) {
            if (n + 1 >= cap) return false;
            out[n++] = bagian[b][i];
        }
    }
    out[n] = '\0';
    return true;
}

// Mengurai isi berkas. false bila formatnya salah.
bool urai_isi(const char* isi, u32* stok, char* rak, u32 rak_cap) {
    const char* awal = "stok=";
    u32 i = 0;
    for (u32 j = 0; awal[j]; j++, i++) {
        if (isi[i] != awal[j]) return false;
    }
    char angka[11];
    u32 n = 0;
    while (isi[i] && isi[i] != ';') {
        if (n >= 10) return false;
        angka[n++] = isi[i++];
    }
    angka[n] = '\0';
    if (isi[i] != ';') return false;
    i++;
    const char* rak_awal = "rak=";
    for (u32 j = 0; rak_awal[j]; j++, i++) {
        if (isi[i] != rak_awal[j]) return false;
    }
    if (!parse_u32(angka, stok)) return false;
    return salin(rak, rak_cap, isi + i);
}

// Baca isi berkas barang ke buf (ukuran ISI_MAX) sebagai string.
int baca_isi(const char* nama, char* buf) {
    int n = fs::read(nama, buf, ISI_MAX - 1);
    if (n < 0) return n;
    if ((u32)n > ISI_MAX - 1) n = (int)(ISI_MAX - 1);
    buf[n] = '\0';
    return n;
}

// ---------- Pesan untuk pengguna ----------
void lapor_galat(int kode, const char* nama) {
    switch (kode) {
        case FS_ENOENT:
            kernel_print("Barang \""); kernel_print(nama); kernel_print("\" tidak ditemukan.\n");
            break;
        case FS_EEXIST:
            kernel_print("Barang \""); kernel_print(nama); kernel_print("\" sudah ada.\n");
            break;
        case FS_ENOMEM:
            kernel_print("Memori habis. Operasi dibatalkan, data lama aman.\n");
            break;
        case FS_EPERM:
            kernel_print("Akses ditolak untuk \""); kernel_print(nama); kernel_print("\".\n");
            break;
        case FS_EFULL:
            kernel_print("Gudang penuh (maks "); kernel_print_dec((u32)FS_MAX_FILES);
            kernel_print(" berkas).\n");
            break;
        case FS_EINVAL:
            kernel_print("Data tidak valid.\n");
            break;
        default:
            kernel_print("Galat tidak dikenal (kode -"); kernel_print_dec((u32)(-kode));
            kernel_print(").\n");
            break;
    }
}

void cetak_kolom(const char* s, u32 lebar) {
    kernel_print(s);
    for (u32 n = panjang(s); n < lebar; n++) kernel_print(" ");
}

// Cetak semua barang yang namanya memuat 'kata' (nullptr/"" = semua). Mengembalikan jumlahnya.
int tampilkan(const char* kata) {
    int jumlah = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        const FsFile* f = fs::at(i);
        if (!f || sama(f->name, LOG_FILE)) continue;
        if (kata && !memuat(f->name, kata)) continue;

        char isi[ISI_MAX];
        char rak[RAK_MAX];
        u32  stok = 0;
        if (baca_isi(f->name, isi) < 0) continue;
        if (!urai_isi(isi, &stok, rak, RAK_MAX)) continue;

        char angka[11];
        angka_ke_teks(stok, angka);
        cetak_kolom(f->name, 32);
        cetak_kolom(angka, 12);
        kernel_print(rak);
        kernel_print("\n");
        jumlah++;
    }
    return jumlah;
}

void cetak_judul() {
    cetak_kolom("NAMA", 32);
    cetak_kolom("STOK", 12);
    kernel_print("RAK\n");
}


// ---------- Tahap 2: thread petugas + mutex ----------
enum Jenis { T_TAMBAH, T_UBAH, T_HAPUS, T_DAFTAR, T_CARI };

struct Tugas {
    Jenis jenis;
    char  nama[FS_NAME_MAX];   // nama barang, atau kata pencarian
    char  rak[RAK_MAX];
    u32   stok;
    int   hasil;               // diisi petugas: 0 atau FsErr
};

Mutex g_mutex;                 // kunci "inventaris"
u32   g_nomor = 1;             // penomoran petugas-N

int kerja_tambah(const Tugas* t) {
    if (!nama_valid(t->nama) || sama(t->nama, LOG_FILE)) {
        kernel_print("Nama barang tidak valid (huruf kecil, angka, atau _ ; maks 31 karakter).\n");
        return FS_EINVAL;
    }
    if (!rak_valid(t->rak)) {
        kernel_print("Rak tidak valid (1-15 karakter, tanpa spasi atau titik koma).\n");
        return FS_EINVAL;
    }
    char isi[ISI_MAX];
    if (!bentuk_isi(t->stok, t->rak, isi, ISI_MAX)) {
        kernel_print("Data barang terlalu panjang.\n");
        return FS_EINVAL;
    }

    syslog::log_s("APP", "tambah barang", t->nama);
    int r = fs::create(t->nama, isi);
    if (r != FS_OK) {
        lapor_galat(r, t->nama);
        return r;
    }
    kernel_print("Barang \""); kernel_print(t->nama);
    kernel_print("\" (stok "); kernel_print_dec(t->stok);
    kernel_print(", rak "); kernel_print(t->rak);
    kernel_print(") ditambahkan.\n");
    return FS_OK;
}

int kerja_ubah(const Tugas* t) {
    char isi[ISI_MAX];
    char rak[RAK_MAX];
    u32  lama = 0;

    syslog::log_s("APP", "ubah stok", t->nama);
    int r = baca_isi(t->nama, isi);
    if (r < 0) { lapor_galat(r, t->nama); return r; }
    if (!urai_isi(isi, &lama, rak, RAK_MAX)) {
        kernel_print("Data barang rusak.\n");
        return FS_EINVAL;
    }

    char baru[ISI_MAX];
    if (!bentuk_isi(t->stok, rak, baru, ISI_MAX)) {
        kernel_print("Data barang terlalu panjang.\n");
        return FS_EINVAL;
    }
    r = fs::write(t->nama, baru);
    if (r != FS_OK) { lapor_galat(r, t->nama); return r; }

    kernel_print("Stok \""); kernel_print(t->nama); kernel_print("\" diubah: ");
    kernel_print_dec(lama); kernel_print(" -> "); kernel_print_dec(t->stok);
    kernel_print(".\n");
    return FS_OK;
}

int kerja_hapus(const Tugas* t) {
    syslog::log_s("APP", "hapus barang", t->nama);
    int r = fs::remove(t->nama);
    if (r != FS_OK) { lapor_galat(r, t->nama); return r; }
    kernel_print("Barang \""); kernel_print(t->nama); kernel_print("\" dihapus.\n");
    return FS_OK;
}

int kerja_daftar() {
    syslog::log("APP", "daftar barang");
    cetak_judul();
    int n = tampilkan(nullptr);
    if (n == 0) kernel_print("(gudang kosong)\n");
    else { kernel_print("Total: "); kernel_print_dec((u32)n); kernel_print(" barang\n"); }
    return FS_OK;
}

int kerja_cari(const Tugas* t) {
    syslog::log_s("APP", "cari barang", t->nama);
    cetak_judul();
    int n = tampilkan(t->nama);
    if (n == 0) {
        kernel_print("Tidak ada barang yang namanya memuat \""); kernel_print(t->nama); kernel_print("\".\n");
    } else {
        kernel_print("Ditemukan: "); kernel_print_dec((u32)n); kernel_print(" barang\n");
    }
    return FS_OK;
}

// Fungsi yang dijalankan thread petugas-N: kunci mutex, kerjakan, lepas, selesai.
void petugas_main(void* arg) {
    Tugas*  t    = (Tugas*)arg;
    Thread* diri = sched::current();

    syslog::log_s("APP", "menunggu mutex inventaris:", diri->name);
    mutex_lock(&g_mutex);
    syslog::log_s("APP", "memegang mutex inventaris:", diri->name);

    switch (t->jenis) {
        case T_TAMBAH: t->hasil = kerja_tambah(t); break;
        case T_UBAH:   t->hasil = kerja_ubah(t);   break;
        case T_HAPUS:  t->hasil = kerja_hapus(t);  break;
        case T_DAFTAR: t->hasil = kerja_daftar();  break;
        case T_CARI:   t->hasil = kerja_cari(t);   break;
    }

    mutex_unlock(&g_mutex);
    syslog::log_s("APP", "melepas mutex inventaris:", diri->name);
    sched::exit();
}

// Buat thread petugas-N untuk satu tugas, tunggu sampai selesai, kembalikan hasilnya.
// (Penunggu ini diganti sched::join bila sudah tersedia dari Anggota 1.)
int jalankan(Tugas* t) {
    char nama_thread[16];
    char angka[11];
    angka_ke_teks(g_nomor % 1000, angka);
    g_nomor++;

    const char* awal = "petugas-";
    u32 n = 0;
    for (; awal[n]; n++) nama_thread[n] = awal[n];
    for (u32 i = 0; angka[i]; i++, n++) nama_thread[n] = angka[i];
    nama_thread[n] = '\0';

    Thread* p = sched::create(nama_thread, petugas_main, t);
    if (!p) {
        kernel_print("Gagal membuat thread petugas (memori atau tabel thread penuh).\n");
        return FS_ENOMEM;
    }

    u32 id = p->id;
    while (p->state != ThreadState::TERMINATED &&
           p->state != ThreadState::UNUSED &&
           p->id == id) {
        sched::yield();
    }
    return t->hasil;
}

}  // namespace

namespace gudang {

void init() {
    if (sudah_init) return;
    sudah_init = true;
    mutex_init(&g_mutex, "inventaris");
    syslog::log("APP", "gudang siap (mutex inventaris dibuat)");
}

int tambah(const char* nama, u32 stok, const char* rak) {
    init();
    Tugas t;
    t.jenis = T_TAMBAH; t.stok = stok; t.hasil = 0;
    if (!salin(t.nama, FS_NAME_MAX, nama) || !salin(t.rak, RAK_MAX, rak)) {
        kernel_print("Nama atau rak terlalu panjang.\n");
        return FS_EINVAL;
    }
    return jalankan(&t);
}

int ubah_stok(const char* nama, u32 stok) {
    init();
    Tugas t;
    t.jenis = T_UBAH; t.stok = stok; t.hasil = 0; t.rak[0] = '\0';
    if (!salin(t.nama, FS_NAME_MAX, nama)) {
        kernel_print("Nama barang terlalu panjang.\n");
        return FS_EINVAL;
    }
    return jalankan(&t);
}

int hapus(const char* nama) {
    init();
    Tugas t;
    t.jenis = T_HAPUS; t.stok = 0; t.hasil = 0; t.rak[0] = '\0';
    if (!salin(t.nama, FS_NAME_MAX, nama)) {
        kernel_print("Nama barang terlalu panjang.\n");
        return FS_EINVAL;
    }
    return jalankan(&t);
}

void daftar() {
    init();
    Tugas t;
    t.jenis = T_DAFTAR; t.stok = 0; t.hasil = 0; t.nama[0] = '\0'; t.rak[0] = '\0';
    jalankan(&t);
}

void cari(const char* kata) {
    init();
    if (!kata || !kata[0]) {
        kernel_print("Pemakaian: cari <kata>\n");
        return;
    }
    Tugas t;
    t.jenis = T_CARI; t.stok = 0; t.hasil = 0; t.rak[0] = '\0';
    if (!salin(t.nama, FS_NAME_MAX, kata)) {
        kernel_print("Kata pencarian terlalu panjang (maks 31 karakter).\n");
        return;
    }
    jalankan(&t);
}

}  // namespace gudang
