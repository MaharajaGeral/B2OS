// GDT: tabel segmen memori. Kita pakai model "flat": kode dan data mencakup seluruh 4 GB.
#include "kernel/gdt.h"
#include "kernel/types.h"

struct GdtEntry {
    u16 limit_low; u16 base_low; u8 base_mid; u8 access; u8 gran; u8 base_high;
} __attribute__((packed));
struct GdtPtr { u16 limit; u32 base; } __attribute__((packed));

extern "C" void gdt_flush(u32 ptr);

static GdtEntry entries[3];
static GdtPtr   gptr;

static void set(int i, u32 base, u32 limit, u8 access, u8 gran) {
    entries[i].base_low  = base & 0xFFFF;
    entries[i].base_mid  = (base >> 16) & 0xFF;
    entries[i].base_high = (base >> 24) & 0xFF;
    entries[i].limit_low = limit & 0xFFFF;
    entries[i].gran      = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    entries[i].access    = access;
}

void gdt::init() {
    set(0, 0, 0, 0, 0);                    // entri null (wajib)
    set(1, 0, 0xFFFFF, 0x9A, 0xCF);        // segmen kode kernel (selector 0x08)
    set(2, 0, 0xFFFFF, 0x92, 0xCF);        // segmen data kernel (selector 0x10)
    gptr.limit = sizeof(entries) - 1;
    gptr.base  = (u32)&entries;
    gdt_flush((u32)&gptr);
}