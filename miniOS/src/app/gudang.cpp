// gudang.cpp - KERANGKA SEMENTARA (akan diganti implementasi asli)
#include "app/gudang.h"
#include "kernel/kernel.h"

namespace gudang {

void init() {}

int tambah(const char* nama, u32 stok, const char* rak) {
    (void)nama; (void)stok; (void)rak;
    kernel_print("[gudang] tambah: belum diimplementasi\n");
    return 0;
}

int ubah_stok(const char* nama, u32 stok) {
    (void)nama; (void)stok;
    kernel_print("[gudang] ubah: belum diimplementasi\n");
    return 0;
}

int hapus(const char* nama) {
    (void)nama;
    kernel_print("[gudang] hapus: belum diimplementasi\n");
    return 0;
}

void daftar() {
    kernel_print("[gudang] daftar: belum diimplementasi\n");
}

void cari(const char* kata) {
    (void)kata;
    kernel_print("[gudang] cari: belum diimplementasi\n");
}

}