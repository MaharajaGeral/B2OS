// kernel.cpp - kernel_main + I/O dasar untuk shell (kriteria 1) + interrupt (persiapan kriteria 3)
#include "kernel/types.h"
#include "kernel/io.h"
#include "kernel/kernel.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/timer.h"
#include "kernel/cpu.h"
#include "proc/scheduler.h"
#include "shell/shell.h"
#include "drivers/serial.h"
#include "drivers/vga.h"

static void putc_both(char c) {
    vga::putc(c);

    if (c == '\n') {
        serial::write('\r');
    }

    serial::write(c);
}

// ---------- Cetak sederhana untuk bagian kernel lain ----------
// Interrupt dimatikan sebentar supaya teks dari beberapa thread tidak tercampur.
void kernel_print(const char* s) {
    u32 f = irq_save();
    while (*s) putc_both(*s++);
    irq_restore(f);
}

void kernel_print_hex(u32 v) {
    u32 f = irq_save();
    putc_both('0'); putc_both('x');
    for (int i = 28; i >= 0; i -= 4) putc_both("0123456789abcdef"[(v >> i) & 0xF]);
    irq_restore(f);
}

void kernel_print_dec(u32 v) {
    char t[11]; int i = 0;
    if (v == 0) t[i++] = '0';
    while (v) { t[i++] = (char)('0' + v % 10); v /= 10; }
    u32 f = irq_save();
    while (i--) putc_both(t[i]);
    irq_restore(f);
}

// ---------- Input: keyboard PS/2 (polling) + serial ----------
static const char NORMAL[] =
    "\0\033" "1234567890-=\b\t" "qwertyuiop[]\n\0" "asdfghjkl;'`\0\\" "zxcvbnm,./\0*\0 ";
static const char SHIFTED[] =
    "\0\033" "!@#$%^&*()_+\b\t" "QWERTYUIOP{}\n\0" "ASDFGHJKL:\"~\0|" "ZXCVBNM<>?\0*\0 ";

static char poll_keyboard() {
    static bool shift = false, ext = false;
    if (!(inb(0x64) & 1)) return 0;
    u8 sc = inb(0x60);
    if (sc == 0xE0) { ext = true; return 0; }
    if (ext)        { ext = false; return 0; }
    if (sc == 0x2A || sc == 0x36) { shift = true;  return 0; }
    if (sc == 0xAA || sc == 0xB6) { shift = false; return 0; }
    if (sc & 0x80) return 0;
    if (sc < sizeof(NORMAL) - 1) return shift ? SHIFTED[sc] : NORMAL[sc];
    return 0;
}

static char read_char() {
    for (;;) {
        char c = poll_keyboard();
        if (c) return c;
        if (serial::data_available()) {
            c = serial::read_byte();

            if (c == '\r') return '\n';
            if (c == 0x7F) return '\b';

            return c;
        }
    }
}

// ---------- Tiga fungsi yang diminta shell (ShellIO) ----------
static void io_write(const char* s) { kernel_print(s); }
static void io_clear() {
    vga::clear();
}

static int io_read_line(char* buf, unsigned long cap) {
    unsigned long n = 0;
    for (;;) {
        char c = read_char();
        if (c == '\n') { putc_both('\n'); buf[n] = 0; return (int)n; }
        if (c == '\b') {
            if (n > 0) { n--; putc_both('\b'); putc_both(' '); putc_both('\b'); }
            continue;
        }
        if (c >= 32 && n < cap - 1) { buf[n++] = c; putc_both(c); }
    }
}

static const ShellIO io = { io_write, io_read_line, io_clear };

extern "C" void kernel_main(u32 magic, u32 info) {
    (void)magic;
    (void)info;

    serial::init();
    vga::clear();

    gdt::init();
    kernel_print("[ OK ] GDT\n");

    idt::init();
    kernel_print("[ OK ] IDT + PIC\n");

    timer::init();
    kernel_print("[ OK ] Timer 100 Hz\n");

    sched::init();
    kernel_print("[ OK ] Scheduler Round Robin\n");

    asm volatile("sti");

    shell_run(&io);

    for (;;) asm volatile("hlt");
}