#include "kernel/system_init.h"

#include "kernel/console.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/timer.h"

#include "proc/scheduler.h"
#include "security/access.h"
#include "fs/fs.h"
#include "app/gudang.h"

#include "drivers/serial.h"
#include "drivers/vga.h"
#include "shell/shell_io.h"

namespace system_init {

void initialize() {
    // Inisialisasi perangkat output
    serial::init();
    vga::clear();

    // Inisialisasi input
    shell_io::init();

    // Inisialisasi komponen kernel
    gdt::init();
    kernel_print("[ OK ] GDT\n");

    idt::init();
    kernel_print("[ OK ] IDT + PIC\n");

    timer::init();
    kernel_print("[ OK ] Timer 100 Hz\n");

    sched::init();
    kernel_print("[ OK ] Scheduler Round Robin\n");

    // Inisialisasi keamanan dan sistem file
    access::init();
    kernel_print("[ OK ] Access Control initialized\n");

    fs::init();
    kernel_print("[ OK ] RAMFS initialized\n");

    gudang::init();
    kernel_print("[ OK ] Gudang initialized\n");

    // Mengaktifkan interrupt
    asm volatile("sti");
}

}