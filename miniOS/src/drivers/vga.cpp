#include "drivers/vga.h"
#include "kernel/types.h"
#include "kernel/io.h"
#include "kernel/cpu.h"

namespace vga {

    // Ukuran layar VGA
    static volatile u16* const VGA = (volatile u16*)0xB8000;

    static const int W = 80;
    static const int H = 25;

    static int row = 0;
    static int col = 0;

    // Warna hijau terang
    static const u8 COLOR = 0x0A;

    // Pojok kanan atas (baris 0, kolom 70-79) dipakai timer untuk menampilkan "UP nnns".
    static const int UPTIME_COL = 70;

    // ---------- Riwayat layar (untuk fitur scroll PageUp/PageDown) ----------
    static const int HIST_LINES = 200;        // jumlah baris riwayat yang disimpan
    static u16 history[HIST_LINES * W];       // baris yang sudah terdorong keluar dari atas layar
    static int hist_count = 0;                // jumlah baris riwayat yang terisi
    static int hist_next = 0;                 // tempat menyimpan baris berikutnya (buffer melingkar)
    static u16 snapshot[W * H];               // salinan layar aktif selama sedang melihat riwayat
    static int view_offset = 0;               // 0 = tampilan normal; n = digulung naik n baris

    // Memperbarui posisi kursor hardware
    static void update_cursor() {
        u16 pos = (u16)(row * W + col);
        outb(0x3D4, 0x0F);
        outb(0x3D5, (u8)(pos & 0xFF));
        outb(0x3D4, 0x0E);
        outb(0x3D5, (u8)((pos >> 8) & 0xFF));
    }

    // Kursor diletakkan di luar layar (tidak terlihat) saat melihat riwayat
    static void hide_cursor() {
        u16 pos = (u16)(W * H);
        outb(0x3D4, 0x0F);
        outb(0x3D5, (u8)(pos & 0xFF));
        outb(0x3D4, 0x0E);
        outb(0x3D5, (u8)((pos >> 8) & 0xFF));
    }

    // Menggambar 25 baris layar dari riwayat + snapshot sesuai view_offset.
    // Baris maya ke-0 = baris riwayat paling lama; setelah riwayat datang 25 baris layar aktif.
    static void render() {
        int first = hist_count - view_offset;

        for (int r = 0; r < H; r++) {
            int v = first + r;

            for (int c = 0; c < W; c++) {
                if (r == 0 && c >= UPTIME_COL) {
                    continue;   // biarkan "UP nnns" dari timer tetap terlihat
                }

                u16 cell;

                if (v < hist_count) {
                    int idx = (hist_next - hist_count + v + HIST_LINES) % HIST_LINES;
                    cell = history[idx * W + c];
                } else {
                    cell = snapshot[(v - hist_count) * W + c];
                }

                VGA[r * W + c] = cell;
            }
        }

        if (view_offset > 0) {
            const char* tag = "[RIWAYAT]";
            for (int i = 0; tag[i] != '\0'; i++) {
                VGA[60 + i] = (u16)(u8)tag[i] | (0x4F << 8);   // putih di merah
            }
        }
    }

    // Kembali ke tampilan normal (paling bawah)
    static void leave_view() {
        if (view_offset > 0) {
            view_offset = 0;
            render();
            update_cursor();
        }
    }

    // Menggulung tampilan: lines > 0 naik, lines < 0 turun
    void scroll_view(int lines) {
        u32 f = irq_save();

        int target = view_offset + lines;
        if (target > hist_count) target = hist_count;
        if (target < 0) target = 0;

        if (target != view_offset) {
            if (view_offset == 0) {
                for (int i = 0; i < W * H; i++) {
                    snapshot[i] = VGA[i];   // simpan layar aktif sebelum ditimpa
                }
            }

            view_offset = target;
            render();

            if (view_offset == 0) {
                update_cursor();
            } else {
                hide_cursor();
            }
        }

        irq_restore(f);
    }

    // Membersihkan layar
    void clear() {
        for (int i = 0; i < W * H; i++) {
            VGA[i] = (u16)' ' | (COLOR << 8);
        }
        row = 0;
        col = 0;
        view_offset = 0;
        hist_count = 0;
        hist_next = 0;
        update_cursor();
    }

    // Menyimpan baris paling atas ke riwayat sebelum terdorong keluar layar
    static void save_top_line() {
        for (int c = 0; c < W; c++) {
            u16 cell = VGA[c];

            if (c >= UPTIME_COL) {
                cell = (u16)' ' | (COLOR << 8);   // bagian ini milik timer, bukan teks
            }

            history[hist_next * W + c] = cell;
        }

        hist_next = (hist_next + 1) % HIST_LINES;

        if (hist_count < HIST_LINES) {
            hist_count++;
        }
    }

    // Menggeser isi layar ke atas
    static void scroll() {
        save_top_line();

        for (int i = 0; i < W * (H - 1); i++) {
            if (i < W && i >= UPTIME_COL) {
                continue;   // jangan timpa "UP nnns" di pojok kanan atas
            }

            VGA[i] = VGA[i + W];
        }

        for (int i = W * (H - 1); i < W * H; i++) {
            VGA[i] = (u16)' ' | (COLOR << 8);
        }

        row = H - 1;
    }

    // Menampilkan satu karakter
    void putc(char c) {
        leave_view();   // ada tulisan baru: kembali ke tampilan paling bawah

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