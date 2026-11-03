#include "shell.h"
#include "logo.h"

void shell_run(const ShellIO* io) {
    show_logo();

    // Memastikan fungsi I/O tersedia
    if (
        !io ||
        !io->write ||
        !io->read_line
    ) {
        return;
    }

    char input[SHELL_BUFFER_SIZE];

    // Pesan pembuka
    io->write(
        "\n"
        "================================\n"
        "       Welcome to B2OS!\n"
        "================================\n"
        "Type 'help' to see available commands.\n\n"
    );

    //Status CLI
    bool running = true;

    // Loop utama CLI
    while (running) {

        io->write("B2OS@localhost:~$ ");

        // Membaca input pengguna
        int result = io->read_line(
            input,
            SHELL_BUFFER_SIZE
        );

        // Menghentikan shell jika input berakhir
        if (result <= 0) {
            break;
        }

        // Memastikan string berakhir dengan null
        input[SHELL_BUFFER_SIZE - 1] = '\0';

        // Menjalankan perintah
        shell_execute_command(input, io, &running);
    }
}