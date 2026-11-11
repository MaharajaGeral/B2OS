#include "shell.h"
#include "logo.h"

static const ShellIO* g_io = nullptr;

// Dipakai logo.cpp dan file lain untuk mencetak lewat I/O yang aktif
void shell_write(const char* text) {
    if (g_io && g_io->write) g_io->write(text);
}

void shell_run(const ShellIO* io) {
    g_io = io;   // harus diset SEBELUM show_logo()

    // Memastikan fungsi I/O tersedia
    if (!io || !io->write || !io->read_line) {
        return;
    }

    show_logo();

    char input[SHELL_BUFFER_SIZE];

    io->write(
        "\n"
        "================================\n"
        "       Welcome to B2OS!\n"
        "================================\n"
        "Type 'help' to see available commands.\n\n"
    );

    bool running = true;

    while (running) {
        io->write("B2OS@localhost:~$ ");

        int result = io->read_line(input, SHELL_BUFFER_SIZE);

        // -1 = input berakhir. Baris kosong (0) tetap lanjut.
        if (result < 0) {
            break;
        }

        input[SHELL_BUFFER_SIZE - 1] = '\0';
        shell_execute_command(input, io, &running);
    }
}