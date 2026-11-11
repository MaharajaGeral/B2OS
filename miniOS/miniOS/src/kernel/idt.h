#pragma once
#include "kernel/types.h"

// Urutan sama dengan yang ditumpuk isr_common (pushad + int_no + err_code + frame CPU)
struct Regs {
    u32 edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    u32 int_no, err_code;
    u32 eip, cs, eflags;
};
typedef void (*IrqHandler)(Regs*);

namespace idt {
void init();                               // isi IDT + inisialisasi PIC
void register_irq(int irq, IrqHandler h);  // irq 0..15
}