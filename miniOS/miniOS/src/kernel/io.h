#pragma once
#include "kernel/types.h"
// Akses port I/O
static inline void outb(u16 port, u8 v) { asm volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
static inline u8   inb(u16 port) { u8 r; asm volatile("inb %1, %0" : "=a"(r) : "Nd"(port)); return r; }