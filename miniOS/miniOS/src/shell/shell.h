#pragma once

#ifndef SHELL_H
#define SHELL_H

#define SHELL_BUFFER_SIZE 128

struct ShellIO {
    void (*write)(const char* text);

    int (*read_line)(
        char* buffer,
        unsigned long capacity
    );

    void (*clear)();
};

//Buat menampilkan teks/logo ke terminal
void shell_write(const char* text);

// Menjalankan CLI
void shell_run(const ShellIO* io);

// Menjalankan perintah
void shell_execute_command(
    const char* input,
    const ShellIO* io,
    bool* running
);

#endif
