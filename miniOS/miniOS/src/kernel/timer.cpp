// Timer PIT: menghasilkan interrupt 100 kali per detik. Inilah "alarm" untuk preemptive scheduler.
#include "kernel/timer.h"
#include "kernel/idt.h"
#include "kernel/io.h"

static volatile u32 tick_count = 0;

// Tampilkan "UP nnns" di pojok kanan atas layar sebagai bukti timer berdetak.
static void draw_uptime() {
    volatile u16* vga = (volatile u16*)0xB8000;
    char buf[10];
    for (int i = 0; i < 10; i++) buf[i] = ' ';
    u32 s = tick_count / timer::HZ;
    buf[0] = 'U'; buf[1] = 'P'; buf[2] = ' ';
    char digits[8]; int n = 0;
    if (s == 0) digits[n++] = '0';
    while (s && n < 6) { digits[n++] = '0' + (s % 10); s /= 10; }
    int pos = 3;
    while (n && pos < 9) buf[pos++] = digits[--n];
    buf[pos] = 's';
    for (int i = 0; i < 10; i++) vga[70 + i] = (u16)(u8)buf[i] | (0x1E << 8);  // kuning di biru
}

static void on_tick(Regs*) {
    tick_count = tick_count + 1;
    if (tick_count % timer::HZ == 0) draw_uptime();
    // Langkah berikutnya (scheduler): di sini nanti dipanggil sched::tick()
}

void timer::init() {
    u32 div = 1193182 / HZ;
    outb(0x43, 0x36);                       // channel 0, mode 3 (square wave)
    outb(0x40, (u8)(div & 0xFF));
    outb(0x40, (u8)((div >> 8) & 0xFF));
    idt::register_irq(0, on_tick);
}

u32 timer::ticks() { return tick_count; }