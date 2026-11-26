#include "kernel/types.h"
#include "kernel/io.h"
#include "kernel/kernel.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/timer.h"
#include "kernel/cpu.h"
#include "kernel/system_init.h"
#include "kernel/console.h"
#include "drivers/keyboard.h"
#include "drivers/serial.h"
#include "drivers/vga.h"
#include "shell/shell.h"
#include "shell/shell_io.h"
#include "proc/scheduler.h"
#include "fs/fs.h"
#include "security/access.h"
#include "app/gudang.h"
#include "security/login_screen.h"

extern "C" void kernel_main(u32 magic, u32 info) {
    (void)magic;
    (void)info;

    system_init::initialize();

    login_screen::show();

    shell_run(shell_io::get());

    for (;;) {
        asm volatile("hlt");
    }
}