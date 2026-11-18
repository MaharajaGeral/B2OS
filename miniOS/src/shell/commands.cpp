#include "shell.h"
#include "proc/demo.h"
#include "shell/args.h"
extern "C" void shutdown_system();

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
        "  ps          - Show thread list and states\n"
        "  thread   - Demo 2 threads and state changes\n"
        "  sched    - Demo preemptive scheduling\n"
        "  race     - Demo race condition vs mutex\n"
        "  sem      - Demo semaphore (2 slots, 4 threads)\n"
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
        "Architecture: x86 (i386, 32-bit)\n"
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

// Uji pengurai argumen: menampilkan tiap token (sementara, untuk pengetesan)
static void command_args(const char* argument, const ShellIO* io) {
    char tokens[ARGS_MAX_TOKENS][ARGS_TOKEN_SIZE];
    int count = args_split(argument, tokens);

    if (count < 0) {
        io->write("Argumen terlalu banyak (maks 4) atau terlalu panjang (maks 31 karakter).\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        io->write("  token ");
        char number[2] = { (char)('1' + i), '\0' };
        io->write(number);
        io->write(": [");
        io->write(tokens[i]);
        io->write("]\n");
    }

    if (count == 0) {
        io->write("  (tidak ada argumen)\n");
    }
}

// trace on|off : menyalakan/mematikan log perubahan status thread
static void command_trace(const char* argument, const ShellIO* io) {
    char tokens[ARGS_MAX_TOKENS][ARGS_TOKEN_SIZE];
    int count = args_split(argument, tokens);

    if (count == 1 && tokens[0][0] == 'o' && tokens[0][1] == 'n' && tokens[0][2] == '\0') {
        trace_enable(true);
        io->write("Trace thread: ON\n");
    } else if (count == 1 && tokens[0][0] == 'o' && tokens[0][1] == 'f' &&
               tokens[0][2] == 'f' && tokens[0][3] == '\0') {
        trace_enable(false);
        io->write("Trace thread: OFF\n");
    } else if (count == 0) {
        io->write(trace_enabled() ? "Trace thread: ON\n" : "Trace thread: OFF\n");
    } else {
        io->write("Pemakaian: trace on|off\n");
    }
}

// Keluar dari shell
static void command_exit(const ShellIO* io) {
    io->write(
        "\nExiting B2OS shell...\n"
        "Shutting down system.\n"
    );
    shutdown_system();
}

// Memproses perintah
void shell_execute_command(
    const char* input,
    const ShellIO* io,
    bool* running
) {
    (void)running;   // belum dipakai; disiapkan untuk perintah yang menghentikan shell
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
    }

    // PS
    else if (
        command_length == 2 &&
        command_matches(command, command_length, "ps")
    ) {
        show_threads();
    }

        // THREAD
    else if (
        command_length == 6 &&
        command_matches(command, command_length, "thread")
    ) {
        demo_thread();
    }

    // SCHED
    else if (
        command_length == 5 &&
        command_matches(command, command_length, "sched")
    ) {
        demo_sched();
    }

    // RACE
    else if (
        command_length == 4 &&
        command_matches(command, command_length, "race")
    ) {
        demo_race();
    }

    // SEM
    else if (
        command_length == 3 &&
        command_matches(command, command_length, "sem")
    ) {
        demo_sem();
    }

    // TRACE
    else if (
        command_length == 5 &&
        command_matches(command, command_length, "trace")
    ) {
        command_trace(input, io);
    }

    // ARGS (uji pengurai argumen)
    else if (
        command_length == 4 &&
        command_matches(command, command_length, "args")
    ) {
        command_args(input, io);
    }

    // COMMAND NOT FOUND
    else {
        io->write(
            "Command not found. Type 'help' for assistance.\n"
        );
    }
}