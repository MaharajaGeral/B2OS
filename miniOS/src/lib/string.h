
#pragma once

#include "kernel/types.h"

extern "C" {

// Mengisi memori dengan nilai tertentu.
void* memset(void* dst, int v, usize n);

// Menyalin data dari sumber ke tujuan.
void* memcpy(void* dst, const void* src, usize n);

// Membandingkan dua blok memori.
int memcmp(const void* a, const void* b, usize n);

// Menghitung panjang string tanpa karakter '\0'.
usize strlen(const char* s);

// Membandingkan dua string.
int strcmp(const char* a, const char* b);

// Membandingkan string maksimal sebanyak n karakter.
int strncmp(const char* a, const char* b, usize n);

// Menyalin string sumber ke tujuan.
char* strcpy(char* dst, const char* src);

// Menyalin string dengan batas maksimal n karakter.
// Selalu menambahkan '\0' pada dst[n-1] jika n > 0.
char* strncpy(char* dst, const char* src, usize n);

}

// Mengubah string angka desimal menjadi u32.
// Mengembalikan false jika string kosong, bukan angka murni,
// atau nilainya melebihi batas u32.
bool parse_u32(const char* s, u32* out);
