#include "shell.h"

// Membandingkan dua string
static bool command_matches(
    const char* command,
    unsigned long length,
    const char* expected
) {
    unsigned long i = 0;

    while (i < length && expected[i] != '\0') {
        if (command[i] != expected[i]) {
            return false;
        }

        i++;
    }

    return i == length && expected[i] == '\0';
}

// Menampilkan daftar perintah
static void command_help(const ShellIO* io) {
    io->write(
        "\nAvailable Commands:\n"
        "  help     - Show available commands\n"
        "  clear    - Clear terminal screen\n"
        "  about    - Information about B2OS\n"
        "  version  - Show OS version\n"
        "  echo     - Display a message\n"
        "  exit     - Exit the shell\n"
        "\n"
    );
}

// Informasi B2OS
static void command_about(const ShellIO* io) {
    io->write(
        "\n"
        "B2OS\n"
        "A simple operating system developed using C++.\n"
        "Architecture: x86_64\n"
        "\n"
    );
}

// Informasi versi
static void command_version(const ShellIO* io) {
    io->write(
        "\nB2OS Version 1.0\n\n"
    );
}

// Menampilkan teks setelah perintah echo
static void command_echo(
    const char* argument,
    const ShellIO* io
) {
    io->write(argument);
    io->write("\n");
}

// Keluar dari shell
static void command_exit(const ShellIO* io) {
    io->write("Exiting B2OS shell...\n");
}

// Memproses perintah
void shell_execute_command(
    const char* input,
    const ShellIO* io,
    bool* running
) {
    if (!input || !io || !io->write) {
        return;
    }

    // Mengabaikan spasi di awal
    while (*input == ' ' || *input == '\t') {
        input++;
    }

    // Input kosong
    if (*input == '\0') {
        return;
    }

    // Membaca nama perintah
    const char* command = input;

    while (
        *input != '\0' &&
        *input != ' ' &&
        *input != '\t'
    ) {
        input++;
    }

    unsigned long command_length = input - command;

    // Mengabaikan spasi setelah nama perintah
    while (*input == ' ' || *input == '\t') {
        input++;
    }

    // HELP
    if (
        command_length == 4 &&
        command_matches(command, command_length, "help")
    ) {
        command_help(io);
    }

    // ABOUT
    else if (
        command_length == 5 &&
        command_matches(command, command_length, "about")
    ) {
        command_about(io);
    }

    // VERSION
    else if (
        command_length == 7 &&
        command_matches(command, command_length, "version")
    ) {
        command_version(io);
    }

    // CLEAR
    else if (
        command_length == 5 &&
        command_matches(command, command_length, "clear")
    ) {
        if (io->clear) {
            io->clear();
        } else {
            io->write(
                "Clear command is not available.\n"
            );
        }
    }

    // ECHO
    else if (
        command_length == 4 &&
        command_matches(command, command_length, "echo")
    ) {
        command_echo(input, io);
    }

    //EXIT
    else if (
        command_length == 4 &&
        command_matches(command, command_length, "exit")
    ) {
        command_exit(io);

        if (running) {
            *running = false;
        }
    }

    // COMMAND NOT FOUND
    else {
        io->write(
            "Command not found. Type 'help' for assistance.\n"
        );
    }
}