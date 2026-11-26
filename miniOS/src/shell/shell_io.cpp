#include "shell/shell_io.h"

#include "kernel/console.h"
#include "drivers/keyboard.h"
#include "drivers/serial.h"
#include "drivers/vga.h"

// Membaca karakter dari keyboard atau serial.
static char read_char() {
    for (;;) {
        char c = keyboard::read_char();

        if (c) {
            return c;
        }

        if (serial::data_available()) {
            c = serial::read_byte();

            if (c == '\r') return '\n';
            if (c == 0x7F) return '\b';

            return c;
        }
    }
}

// Mengirim teks ke console.
void shell_io::write(const char* text) {
    kernel_print(text);
}

// Membersihkan layar.
void shell_io::clear() {
    vga::clear();
}

// Membaca satu baris input.
int shell_io::read_line(char* buffer, unsigned long capacity) {
    if (!buffer || capacity == 0) {
        return -1;
    }

    unsigned long n = 0;

    for (;;) {
        char c = read_char();

        if (c == '\n') {
            console_putc('\n');
            buffer[n] = '\0';
            return (int)n;
        }

        if (c == '\b') {
            if (n > 0) {
                n--;

                console_putc('\b');
                console_putc(' ');
                console_putc('\b');
            }

            continue;
        }

        if (c >= 32 && n < capacity - 1) {
            buffer[n++] = c;
            console_putc(c);
        }
    }
}

// Inisialisasi keyboard.
void shell_io::init() {
    keyboard::init();
}

// Mengambil konfigurasi ShellIO.
const ShellIO* shell_io::get() {
    static const ShellIO io = {
        shell_io::write,
        shell_io::read_line,
        shell_io::clear
    };

    return &io;
}