// IDT: tabel yang memberi tahu CPU fungsi mana yang dipanggil untuk tiap interrupt.
#include "kernel/idt.h"
#include "kernel/pic.h"
#include "kernel/kernel.h"

struct IdtEntry {
    u16 off_low; u16 sel; u8 zero; u8 flags; u16 off_high;
} __attribute__((packed));
struct IdtPtr { u16 limit; u32 base; } __attribute__((packed));

extern "C" u32 isr_stub_table[48];

static IdtEntry   entries[256];
static IdtPtr     iptr;
static IrqHandler handlers[16];

static const char* exc_names[32] = {
    "Divide Error", "Debug", "NMI", "Breakpoint", "Overflow", "Bound Range",
    "Invalid Opcode", "Device Not Available", "Double Fault", "Coprocessor Overrun",
    "Invalid TSS", "Segment Not Present", "Stack Fault", "General Protection",
    "Page Fault", "Reserved", "x87 FPU Error", "Alignment Check", "Machine Check",
    "SIMD FP", "Virtualization", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Security", "Reserved"
};

static void set_gate(int i, u32 handler) {
    entries[i].off_low  = handler & 0xFFFF;
    entries[i].sel      = 0x08;            // segmen kode kernel
    entries[i].zero     = 0;
    entries[i].flags    = 0x8E;            // present, ring 0, interrupt gate
    entries[i].off_high = (handler >> 16) & 0xFFFF;
}

void idt::init() {
    pic::init();
    for (int i = 0; i < 48; i++) set_gate(i, isr_stub_table[i]);
    iptr.limit = sizeof(entries) - 1;
    iptr.base  = (u32)&entries;
    asm volatile("lidt %0" : : "m"(iptr));
}

void idt::register_irq(int irq, IrqHandler h) { handlers[irq] = h; }

// Dipanggil dari isr_common (isr_stubs.S) untuk setiap interrupt.
extern "C" void isr_handler(Regs* r) {
    if (r->int_no < 32) {                  // exception CPU -> kernel panic
        kernel_print("\n*** KERNEL PANIC: exception ");
        kernel_print_hex(r->int_no);
        kernel_print(" (");
        kernel_print(exc_names[r->int_no]);
        kernel_print(") ***\neip=");
        kernel_print_hex(r->eip);
        kernel_print(" err=");
        kernel_print_hex(r->err_code);
        kernel_print("\n");
        for (;;) asm volatile("cli; hlt");
    }
    int irq = (int)r->int_no - 32;
    pic::eoi(irq);                         // EOI dulu (penting untuk context switch nanti)
    if (handlers[irq]) handlers[irq](r);
}