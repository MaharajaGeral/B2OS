
#include "app/gudang.h"
#include "kernel/kernel.h"
#include "fs/fs.h"

namespace gudang {

static const char* FILE_GUDANG = "gudang.txt";
static const u32 BUFFER_SIZE = 4096;

static char buffer[BUFFER_SIZE];
static char temp[BUFFER_SIZE];

// Menghitung panjang string
static u32 panjang(const char* s) {
    u32 n = 0;
    while (s[n]) n++;
    return n;
}

// Menyalin string
static void salin(char* tujuan, const char* sumber) {
    u32 i = 0;
    while (sumber[i]) {
        tujuan[i] = sumber[i];
        i++;
    }
    tujuan[i] = '\0';
}

// Membandingkan dua string
static bool sama(const char* a, const char* b) {
    u32 i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return false;
        i++;
    }
    return a[i] == b[i];
}

// Memeriksa apakah teks mengandung kata
static bool mengandung(const char* teks, const char* kata) {
    if (!*kata) return true;

    for (u32 i = 0; teks[i]; i++) {
        u32 j = 0;

        while (kata[j] && teks[i + j] == kata[j]) {
            j++;
        }

        if (!kata[j]) return true;
    }

    return false;
}

// Mengubah angka menjadi teks
static void angka_ke_teks(u32 angka, char* hasil) {
    char sementara[11];
    u32 n = 0;

    do {
        sementara[n++] = '0' + angka % 10;
        angka /= 10;
    } while (angka);

    for (u32 i = 0; i < n; i++) {
        hasil[i] = sementara[n - 1 - i];
    }

    hasil[n] = '\0';
}

// Membaca data dari RAMFS
static int muat_data() {
    int hasil = fs::read(FILE_GUDANG, buffer, BUFFER_SIZE);

    if (hasil < 0) return hasil;

    return FS_OK;
}

// Menyimpan data ke RAMFS
static int simpan_data() {
    return fs::write(FILE_GUDANG, buffer);
}

// Memeriksa karakter pemisah agar format data tetap valid
static bool valid_teks(const char* teks) {
    if (!teks || !teks[0]) return false;

    for (u32 i = 0; teks[i]; i++) {
        if (teks[i] == '|' || teks[i] == '\n' ||
            teks[i] == '\r') {
            return false;
        }
    }

    return true;
}

// Memeriksa apakah nama barang sudah ada
static bool barang_ada(const char* nama) {
    u32 pos = 0;

    while (buffer[pos]) {
        char nama_barang[FS_NAME_MAX];
        u32 i = 0;

        while (buffer[pos] && buffer[pos] != '|') {
            if (i < FS_NAME_MAX - 1) {
                nama_barang[i++] = buffer[pos];
            }
            pos++;
        }

        nama_barang[i] = '\0';

        if (sama(nama_barang, nama)) return true;

        while (buffer[pos] && buffer[pos] != '\n') pos++;
        if (buffer[pos] == '\n') pos++;
    }

    return false;
}

// Inisialisasi data gudang
void init() {
    int hasil = fs::read(FILE_GUDANG, buffer, BUFFER_SIZE);

    if (hasil == FS_ENOENT) {
        fs::create(FILE_GUDANG, "");
    }
}

// Menambahkan barang
int tambah(const char* nama, u32 stok, const char* rak) {
    if (!valid_teks(nama) || !valid_teks(rak)) {
        return FS_EINVAL;
    }

    if (panjang(nama) >= FS_NAME_MAX ||
        panjang(rak) >= FS_NAME_MAX) {
        return FS_EINVAL;
    }

    int hasil = muat_data();
    if (hasil != FS_OK) return hasil;

    if (barang_ada(nama)) return FS_EEXIST;

    u32 akhir = panjang(buffer);

    char teks_stok[11];
    angka_ke_teks(stok, teks_stok);

    u32 tambahan = panjang(nama) + panjang(teks_stok)
                 + panjang(rak) + 3;

    if (akhir + tambahan >= BUFFER_SIZE) {
        return FS_EFULL;
    }

    u32 pos = akhir;

    if (akhir > 0 && buffer[akhir - 1] != '\n') {
        buffer[pos++] = '\n';
    }

    for (u32 i = 0; nama[i]; i++) buffer[pos++] = nama[i];
    buffer[pos++] = '|';

    for (u32 i = 0; teks_stok[i]; i++) buffer[pos++] = teks_stok[i];
    buffer[pos++] = '|';

    for (u32 i = 0; rak[i]; i++) buffer[pos++] = rak[i];
    buffer[pos++] = '\n';
    buffer[pos] = '\0';

    return simpan_data();
}

// Mengubah stok barang
int ubah_stok(const char* nama, u32 stok) {
    if (!nama || !nama[0]) return FS_EINVAL;

    int hasil = muat_data();
    if (hasil != FS_OK) return hasil;

    temp[0] = '\0';

    u32 pos = 0;
    u32 keluar = 0;
    bool ditemukan = false;

    char teks_stok[11];
    angka_ke_teks(stok, teks_stok);

    while (buffer[pos]) {
        u32 awal = pos;
        u32 nama_akhir = pos;

        while (buffer[pos] && buffer[pos] != '|') pos++;
        nama_akhir = pos;

        if (buffer[pos] == '|') pos++;

        while (buffer[pos] && buffer[pos] != '|') pos++;
        if (buffer[pos] == '|') pos++;

        while (buffer[pos] && buffer[pos] != '\n') pos++;

        u32 akhir = pos;
        if (buffer[pos] == '\n') pos++;

        char nama_barang[FS_NAME_MAX];
        u32 n = nama_akhir - awal;

        if (n >= FS_NAME_MAX) return FS_EINVAL;

        for (u32 i = 0; i < n; i++) {
            nama_barang[i] = buffer[awal + i];
        }
        nama_barang[n] = '\0';

        if (sama(nama_barang, nama)) {
            ditemukan = true;

            for (u32 i = 0; i < n; i++) temp[keluar++] = buffer[awal + i];
            temp[keluar++] = '|';

            for (u32 i = 0; teks_stok[i]; i++) temp[keluar++] = teks_stok[i];
            temp[keluar++] = '|';

            for (u32 i = awal; i < akhir; i++) {
                if (buffer[i] == '|') {
                    i++;
                    while (i < akhir && buffer[i] != '|') i++;
                    if (i < akhir) {
                        i++;
                        while (i < akhir) temp[keluar++] = buffer[i++];
                    }
                    break;
                }
            }

            if (buffer[akhir] == '\n') temp[keluar++] = '\n';
        } else {
            for (u32 i = awal; i < pos; i++) {
                temp[keluar++] = buffer[i];
            }
        }

        if (keluar >= BUFFER_SIZE - 1) return FS_EFULL;
    }

    if (!ditemukan) return FS_ENOENT;

    temp[keluar] = '\0';
    salin(buffer, temp);

    return simpan_data();
}

// Menghapus barang
int hapus(const char* nama) {
    if (!nama || !nama[0]) return FS_EINVAL;

    int hasil = muat_data();
    if (hasil != FS_OK) return hasil;

    temp[0] = '\0';

    u32 pos = 0;
    u32 keluar = 0;
    bool ditemukan = false;

    while (buffer[pos]) {
        u32 awal = pos;

        while (buffer[pos] && buffer[pos] != '|') pos++;

        u32 n = pos - awal;

        char nama_barang[FS_NAME_MAX];
        if (n >= FS_NAME_MAX) return FS_EINVAL;

        for (u32 i = 0; i < n; i++) {
            nama_barang[i] = buffer[awal + i];
        }
        nama_barang[n] = '\0';

        while (buffer[pos] && buffer[pos] != '\n') pos++;
        if (buffer[pos] == '\n') pos++;

        if (sama(nama_barang, nama)) {
            ditemukan = true;
        } else {
            for (u32 i = awal; i < pos; i++) {
                temp[keluar++] = buffer[i];
            }
        }

        if (keluar >= BUFFER_SIZE - 1) return FS_EFULL;
    }

    if (!ditemukan) return FS_ENOENT;

    temp[keluar] = '\0';
    salin(buffer, temp);

    return simpan_data();
}

// Menampilkan seluruh barang
void daftar() {
    if (muat_data() != FS_OK) {
        kernel_print("Gagal membaca data gudang.\n");
        return;
    }

    if (!buffer[0]) {
        kernel_print("Belum ada barang.\n");
        return;
    }

    kernel_print("\nNama | Stok | Rak\n");
    kernel_print("-------------------------\n");
    kernel_print(buffer);
}

// Mencari barang berdasarkan nama
void cari(const char* kata) {
    if (!kata || !kata[0]) {
        kernel_print("Masukkan nama barang yang dicari.\n");
        return;
    }

    if (muat_data() != FS_OK) {
        kernel_print("Gagal membaca data gudang.\n");
        return;
    }

    bool ditemukan = false;
    u32 pos = 0;

    while (buffer[pos]) {
        u32 awal = pos;

        while (buffer[pos] && buffer[pos] != '\n') pos++;

        u32 akhir = pos;

        char baris[128];
        u32 n = akhir - awal;

        if (n >= sizeof(baris)) return;

        for (u32 i = 0; i < n; i++) {
            baris[i] = buffer[awal + i];
        }
        baris[n] = '\0';

        if (mengandung(baris, kata)) {
            kernel_print(baris);
            kernel_print("\n");
            ditemukan = true;
        }

        if (buffer[pos] == '\n') pos++;
    }

    if (!ditemukan) {
        kernel_print("Barang tidak ditemukan.\n");
    }
}

}
