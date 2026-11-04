// PIC 8259: pengatur interrupt perangkat keras.
#include "kernel/pic.h"
#include "kernel/io.h"

static inline void io_wait() { outb(0x80, 0); }

void pic::init() {
    outb(0x20, 0x11); io_wait();   // ICW1: mulai inisialisasi
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();   // ICW2: IRQ0-7  -> vektor 32-39
    outb(0xA1, 0x28); io_wait();   //       IRQ8-15 -> vektor 40-47
    outb(0x21, 0x04); io_wait();   // ICW3: hubungan master-slave
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();   // ICW4: mode 8086
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xFE);              // hanya IRQ0 (timer) yang aktif
    outb(0xA1, 0xFF);
}

void pic::eoi(int irq) {
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}