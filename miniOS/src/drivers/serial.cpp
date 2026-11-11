#include "drivers/serial.h"
#include "kernel/io.h"

namespace serial {

    void init() {
        outb(0x3F9, 0x00);
        outb(0x3FB, 0x80);
        outb(0x3F8, 0x03);
        outb(0x3F9, 0x00);
        outb(0x3FB, 0x03);
        outb(0x3FA, 0xC7);
        outb(0x3FC, 0x0B);
    }

    void write(char c) {
        while (!(inb(0x3FD) & 0x20)) {}
        outb(0x3F8, (u8)c);
    }

    bool data_available() {
        return (inb(0x3FD) & 1) != 0;
    }

    char read_byte() {
        return (char)inb(0x3F8);
    }

}