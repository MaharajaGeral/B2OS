#pragma once
#include "kernel/types.h"
// Matikan interrupt, kembalikan flag lama (aman dipakai bersarang)
static inline u32 irq_save() {
    u32 f;
    asm volatile("pushf; pop %0; cli" : "=r"(f) : : "memory");
    return f;
}
static inline void irq_restore(u32 f) {
    if (f & 0x200) asm volatile("sti" : : : "memory");
}