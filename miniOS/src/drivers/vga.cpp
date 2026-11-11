#include "drivers/vga.h"
#include "kernel/types.h"
#include "kernel/io.h"

namespace vga {

    // Ukuran layar VGA
    static volatile u16* const VGA = (volatile u16*)0xB8000;

    static const int W = 80;
    static const int H = 25;

    static int row = 0;
    static int col = 0;

    // Warna hijau terang
    static const u8 COLOR = 0x0A;

    // Memperbarui posisi kursor hardware
    static void update_cursor() {
        u16 pos = (u16)(row * W + col);
        outb(0x3D4, 0x0F);
        outb(0x3D5, (u8)(pos & 0xFF));
        outb(0x3D4, 0x0E);
        outb(0x3D5, (u8)((pos >> 8) & 0xFF));
    }

    // Membersihkan layar
    void clear() {
        for (int i = 0; i < W * H; i++) {
            VGA[i] = (u16)' ' | (COLOR << 8);
        }
        row = 0;
        col = 0;
        update_cursor();
    }

    // Menggeser isi layar ke atas
    static void scroll() {
        for (int i = 0; i < W * (H - 1); i++) {
            VGA[i] = VGA[i + W];
        }

        for (int i = W * (H - 1); i < W * H; i++) {
            VGA[i] = (u16)' ' | (COLOR << 8);
        }

        row = H - 1;
    }

    // Menampilkan satu karakter
    void putc(char c) {
        if (c == '\n') {
            col = 0;
            row++;
        }
        else if (c == '\r') {
            col = 0;
        }
        else if (c == '\b') {
            if (col > 0) {
                col--;
                VGA[row * W + col] = (u16)' ' | (COLOR << 8);
            }
        }
        else {
            VGA[row * W + col] = (u16)(u8)c | (COLOR << 8);
            col++;
        }

        if (col >= W) {
            col = 0;
            row++;
        }

        if (row >= H) {
            scroll();
        }

        update_cursor();
    }

}