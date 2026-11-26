#pragma once
#include "kernel/types.h"

// Output karakter ke VGA dan Serial.
void console_putc(char c);

// Output teks.
void kernel_print(const char* s);

// Output angka heksadesimal.
void kernel_print_hex(u32 v);

// Output angka desimal.
void kernel_print_dec(u32 v);
