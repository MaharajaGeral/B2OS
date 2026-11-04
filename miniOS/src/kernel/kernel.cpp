// kernel.cpp - kernel_main + I/O dasar untuk shell (kriteria 1) + interrupt (persiapan kriteria 3)
#include "kernel/types.h"
#include "kernel/io.h"
#include "kernel/kernel.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/timer.h"
#include "shell/shell.h"

// ---------- Serial COM1 ----------
static void serial_init() {
    outb(0x3F9, 0x00); outb(0x3FB, 0x80); outb(0x3F8, 0x03);
    outb(0x3F9, 0x00); outb(0x3FB, 0x03); outb(0x3FA, 0xC7); outb(0x3FC, 0x0B);
}
static void serial_putc(char c) {
    while (!(inb(0x3FD) & 0x20)) {}
    outb(0x3F8, (u8)c);
}

// ---------- Layar VGA 80x25 ----------
static volatile u16* const VGA = (volatile u16*)0xB8000;
static const int W = 80, H = 25;
static int row = 0, col = 0;
static const u8 COLOR = 0x0A;                       // hijau terang

static void vga_clear() {
    for (int i = 0; i < W * H; i++) VGA[i] = (u16)' ' | (COLOR << 8);
    row = 0; col = 0;
}
static void vga_scroll() {
    for (int i = 0; i < W * (H - 1); i++) VGA[i] = VGA[i + W];
    for (int i = W * (H - 1); i < W * H; i++) VGA[i] = (u16)' ' | (COLOR << 8);
    row = H - 1;
}
static void vga_putc(char c) {
    if (c == '\n')      { col = 0; row++; }
    else if (c == '\r') { col = 0; }
    else if (c == '\b') { if (col > 0) { col--; VGA[row * W + col] = (u16)' ' | (COLOR << 8); } }
    else                { VGA[row * W + col] = (u16)(u8)c | (COLOR << 8); col++; }
    if (col >= W) { col = 0; row++; }
    if (row >= H) vga_scroll();
}

static void putc_both(char c) {
    vga_putc(c);
    if (c == '\n') serial_putc('\r');
    serial_putc(c);
}

// ---------- Cetak sederhana untuk bagian kernel lain ----------
void kernel_print(const char* s) { while (*s) putc_both(*s++); }

void kernel_print_hex(u32 v) {
    kernel_print("0x");
    for (int i = 28; i >= 0; i -= 4) putc_both("0123456789abcdef"[(v >> i) & 0xF]);
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
        if (inb(0x3FD) & 1) {                       // ada data dari serial (terminal)
            c = (char)inb(0x3F8);
            if (c == '\r') return '\n';
            if (c == 0x7F) return '\b';
            return c;
        }
    }
}

// ---------- Tiga fungsi yang diminta shell (ShellIO) ----------
static void io_write(const char* s) { while (*s) putc_both(*s++); }
static void io_clear() { vga_clear(); }

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
    (void)magic; (void)info;
    serial_init();
    vga_clear();

    gdt::init();      kernel_print("[ OK ] GDT\n");
    idt::init();      kernel_print("[ OK ] IDT + PIC\n");
    timer::init();    kernel_print("[ OK ] Timer 100 Hz\n");
    asm volatile("sti");                            // mulai menerima interrupt

    shell_run(&io);                 // jalankan CLI
    for (;;) asm volatile("hlt");   // kalau shell berhenti, timer tetap berdetak
}