#include <stdint.h>

extern "C" void shutdown_system() {

    uint32_t code = 0x10;

    asm volatile(
        "outl %0, %1"
        :
        : "a"(code), "Nd"(0xF4)
    );

    // Jika QEMU tidak merespons,
    // hentikan CPU sementara.
    asm volatile("cli");

    while (1) {
        asm volatile("hlt");
    }
}