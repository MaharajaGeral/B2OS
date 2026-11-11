#include <iostream>
#include <limits>
#include "../../src/shell/shell.h"

// Menampilkan teks ke terminal komputer
void host_write(const char* text) {
    std::cout << text;
}

// Membaca input dari keyboard
int host_read_line(
    char* buffer,
    unsigned long capacity
) {
    if (capacity < 2) {
        return 0;
    }

    if (!std::cin.getline(
        buffer,
        static_cast<std::streamsize>(capacity)
    )) {

        // Jika pengguna mengakhiri input
        if (std::cin.eof()) {
            return 0;
        }

        // Jika input terlalu panjang
        std::cin.clear();

        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(),
            '\n'
        );

        buffer[capacity - 1] = '\0';
    }

    return 1;
}

// Membersihkan layar terminal
void host_clear() {
    std::cout << "\033[2J\033[H";
}

// Menghubungkan fungsi host dengan CLI
int main() {

    ShellIO host_io = {
        host_write,
        host_read_line,
        host_clear
    };

    // Menjalankan CLI B2OS
    shell_run(&host_io);

    return 0;
}