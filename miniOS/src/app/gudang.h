// Hanya deklarasi. Isi fungsi ada di gudang.cpp.
#pragma once
#include "kernel/types.h"

namespace gudang {
void init();                                                 // buat mutex/semaphore + thread latar
int  tambah(const char* nama, u32 stok, const char* rak);    // 0 atau FsErr
int  ubah_stok(const char* nama, u32 stok);                  // 0 atau FsErr
int  hapus(const char* nama);                                // 0 atau FsErr
void daftar();                                               // cetak semua barang (tabel)
void cari(const char* kata);                                 // cetak barang yang namanya memuat 'kata'
}