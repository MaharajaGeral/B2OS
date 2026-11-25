#include "shell.h"
#include "proc/demo.h"
#include "shell/args.h"
#include "fs/fs.h"
#include "app/gudang.h"
#include "security/access.h"
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
// Mengubah teks angka menjadi u32
static bool parse_u32(const char* text, u32* result) {
    if (!text || !result || *text == '\0') {
        return false;
    }

    u32 value = 0;

    while (*text) {
        if (*text < '0' || *text > '9') {
            return false;
        }

        u32 digit = (u32)(*text - '0');

        if (value > (0xFFFFFFFFu - digit) / 10) {
            return false;
        }

        value = value * 10 + digit;
        text++;
    }

    *result = value;
    return true;
}
// Menambahkan barang ke inventaris gudang
static void command_tambah(const char* argument, const ShellIO* io) {
    if (access::current_uid() != access::UID_ADMIN) {
        io->write("Akses ditolak: hanya admin yang boleh menambahkan barang.\n");
        return;
    }

    char tokens[ARGS_MAX_TOKENS][ARGS_TOKEN_SIZE];
    int count = args_split(argument, tokens);

    if (count != 3) {
        io->write("Pemakaian: tambah <nama> <stok> <rak>\n");
        return;
    }

    u32 stok;

    if (!parse_u32(tokens[1], &stok)) {
        io->write("Stok harus berupa angka yang valid.\n");
        return;
    }

    int result = gudang::tambah(tokens[0], stok, tokens[2]);

    if (result == FS_OK) {
        io->write("Barang berhasil ditambahkan.\n");
    } else if (result == FS_EEXIST) {
        io->write("Barang sudah terdaftar.\n");
    } else if (result == FS_ENOMEM || result == FS_EFULL) {
        io->write("Penyimpanan gudang penuh.\n");
    } else {
        io->write("Gagal menambahkan barang.\n");
    }
}
// Menampilkan seluruh barang di gudang
static void command_daftar(const ShellIO* io) {
    io->write("\n=== DAFTAR BARANG GUDANG ===\n");

    gudang::daftar();

    io->write("============================\n\n");
}
// Mencari barang berdasarkan nama
static void command_cari(const char* argument, const ShellIO* io) {
    char tokens[ARGS_MAX_TOKENS][ARGS_TOKEN_SIZE];
    int count = args_split(argument, tokens);

    if (count != 1) {
        io->write("Pemakaian: cari <nama>\n");
        return;
    }

    io->write("\n=== HASIL PENCARIAN ===\n");
    gudang::cari(tokens[0]);
    io->write("=======================\n\n");
}
// Mengubah jumlah stok barang
static void command_stok(const char* argument, const ShellIO* io) {
    if (access::current_uid() != access::UID_ADMIN) {
        io->write("Akses ditolak: hanya admin yang boleh mengubah stok.\n");
        return;
    }

    char tokens[ARGS_MAX_TOKENS][ARGS_TOKEN_SIZE];
    int count = args_split(argument, tokens);

    if (count != 2) {
        io->write("Pemakaian: stok <nama> <jumlah_baru>\n");
        return;
    }

    u32 stok;

    if (!parse_u32(tokens[1], &stok)) {
        io->write("Stok harus berupa angka yang valid.\n");
        return;
    }

    int result = gudang::ubah_stok(tokens[0], stok);

    if (result == FS_OK) {
        io->write("Stok barang berhasil diperbarui.\n");
    } else if (result == FS_ENOENT) {
        io->write("Barang tidak ditemukan.\n");
    } else {
        io->write("Gagal memperbarui stok barang.\n");
    }
}
// Menghapus barang dari inventaris gudang
static void command_hapus(const char* argument, const ShellIO* io) {
    if (access::current_uid() != access::UID_ADMIN) {
        io->write("Akses ditolak: hanya admin yang boleh menghapus barang.\n");
        return;
    }

    char tokens[ARGS_MAX_TOKENS][ARGS_TOKEN_SIZE];
    int count = args_split(argument, tokens);

    if (count != 1) {
        io->write("Pemakaian: hapus <nama>\n");
        return;
    }

    int result = gudang::hapus(tokens[0]);

    if (result == FS_OK) {
        io->write("Barang berhasil dihapus.\n");
    } else if (result == FS_ENOENT) {
        io->write("Barang tidak ditemukan.\n");
    } else {
        io->write("Gagal menghapus barang.\n");
    }
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
        "  fs       - Test RAM filesystem operations\n"
        "  tambah   - Add warehouse item (name stock rack)\n"
        "  daftar   - Show warehouse inventory\n"
        "  cari     - Search warehouse item\n"
        "  stok     - Update item stock\n"
        "  hapus    - Delete warehouse item\n"
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

// Menguji operasi dasar RAMFS
static void command_fs(const ShellIO* io) {
    char buffer[128] = {};

    io->write("\n=== RAMFS TEST ===\n");

    // Membersihkan file sisa pengujian sebelumnya
    fs::remove("test.txt");

    // 1. Membuat file
    io->write("[1] Create file: ");

    int result = fs::create("test.txt", "B2OS RAMFS Test");

    if (result == FS_OK) {
        io->write("SUCCESS\n");
    } else {
        io->write("FAILED\n");
        return;
    }

    // 2. Membaca file
    io->write("[2] Read file: ");

    result = fs::read("test.txt", buffer, sizeof(buffer));

    if (result >= 0) {
        io->write("SUCCESS\n");
        io->write("    Content: ");
        io->write(buffer);
        io->write("\n");
    } else {
        io->write("FAILED\n");
        return;
    }

    // 3. Mengubah isi file
    io->write("[3] Write file: ");

    result = fs::write("test.txt", "RAMFS successfully updated");

    if (result == FS_OK) {
        io->write("SUCCESS\n");
    } else {
        io->write("FAILED\n");
        return;
    }

    // 4. Membaca kembali isi file
    io->write("[4] Read updated file: ");

    result = fs::read("test.txt", buffer, sizeof(buffer));

    if (result >= 0) {
        io->write("SUCCESS\n");
        io->write("    Content: ");
        io->write(buffer);
        io->write("\n");
    } else {
        io->write("FAILED\n");
        return;
    }

    // 5. Menghapus file
    io->write("[5] Remove file: ");

    result = fs::remove("test.txt");

    if (result == FS_OK) {
        io->write("SUCCESS\n");
    } else {
        io->write("FAILED\n");
        return;
    }

    // Memastikan file sudah terhapus
if (fs::find("test.txt") == nullptr) {
    io->write("[6] Verify deletion: SUCCESS\n");
} else {
    io->write("[6] Verify deletion: FAILED\n");
}

io->write("=== RAMFS TEST FINISHED ===\n\n");
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
        // FILESYSTEM
    else if (
        command_length == 2 &&
        command_matches(command, command_length, "fs")
    ) {
        command_fs(io);
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

        // WAREHOUSE
    else if (
        command_length == 6 &&
        command_matches(command, command_length, "tambah")
    ) {
        command_tambah(input, io);
    }

    else if (
        command_length == 6 &&
        command_matches(command, command_length, "daftar")
    ) {
        command_daftar(io);
    }

    else if (
        command_length == 4 &&
        command_matches(command, command_length, "cari")
    ) {
        command_cari(input, io);
    }

    else if (
        command_length == 4 &&
        command_matches(command, command_length, "stok")
    ) {
        command_stok(input, io);
    }

    else if (
        command_length == 5 &&
        command_matches(command, command_length, "hapus")
    ) {
        command_hapus(input, io);
    }

    // COMMAND NOT FOUND
    else {
        io->write(
            "Command not found. Type 'help' for assistance.\n"
        );
    }
}