#include "kernel/console.h"
#include "kernel/cpu.h"
#include "drivers/serial.h"
#include "drivers/vga.h"

// Output satu karakter ke VGA dan Serial.
static void putc_both(char c) {
    vga::putc(c);

    if (c == '\n') {
        serial::write('\r');
    }

    serial::write(c);
}

// Output satu karakter yang dapat digunakan file lain.
void console_putc(char c) {
    u32 f = irq_save();
    putc_both(c);
    irq_restore(f);
}

// Output teks.
void kernel_print(const char* s) {
    u32 f = irq_save();

    while (*s) {
        putc_both(*s++);
    }

    irq_restore(f);
}

// Output angka heksadesimal.
void kernel_print_hex(u32 v) {
    u32 f = irq_save();

    putc_both('0');
    putc_both('x');

    for (int i = 28; i >= 0; i -= 4) {
        putc_both("0123456789abcdef"[(v >> i) & 0xF]);
    }

    irq_restore(f);
}

// Output angka desimal.
void kernel_print_dec(u32 v) {
    char t[11];
    int i = 0;

    if (v == 0) {
        t[i++] = '0';
    }

    while (v) {
        t[i++] = (char)('0' + v % 10);
        v /= 10;
    }

    u32 f = irq_save();

    while (i--) {
        putc_both(t[i]);
    }

    irq_restore(f);
}
