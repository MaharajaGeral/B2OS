#pragma once
#include "kernel/types.h"
// Fungsi cetak sederhana yang dipakai bagian kernel lain
void kernel_print(const char* s);
void kernel_print_hex(u32 v);
void kernel_print_dec(u32 v);