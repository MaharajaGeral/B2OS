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

    // ---------- Salinan teks layar ----------
    // Semua teks ditulis ke screen[] lebih dulu. VGA hanya menampilkan isi screen[], kecuali
    // area yang sedang tertutup pop-up. Dengan begitu teks yang tercetak saat pop-up tampil
    // tidak hilang ketika pop-up ditutup.
    static u16 screen[W * H];
    static bool screen_ready = false;

    static void ensure_screen() {
        if (screen_ready) return;
        for (int i = 0; i < W * H; i++) {
            screen[i] = VGA[i];   // ambil isi layar yang sudah ada sekali saja
        }
        screen_ready = true;
    }

    // ---------- Pop-up (overlay) ----------
    static bool ov_on = false;
    static int  ov_top = 0, ov_left = 0, ov_h = 0, ov_w = 0;

    static bool covered(int r, int c) {
        return ov_on && r >= ov_top && r < ov_top + ov_h && c >= ov_left && c < ov_left + ov_w;
    }

    // Menulis satu sel teks: selalu ke screen[], ke VGA hanya bila tidak tertutup pop-up
    static void put_cell(int r, int c, u16 cell) {
        screen[r * W + c] = cell;
        if (!covered(r, c)) {
            VGA[r * W + c] = cell;
        }
    }

    // Menggambar ulang seluruh layar dari screen[] (kecuali area pop-up dan jam "UP")
    static void repaint() {
        for (int r = 0; r < H; r++) {
            for (int c = 0; c < W; c++) {
                if (r == 0 && c >= UPTIME_COL) continue;
                if (covered(r, c)) continue;
                VGA[r * W + c] = screen[r * W + c];
            }
        }
    }

    // Menutup pop-up (pemanggil sudah menonaktifkan interrupt)
    static void overlay_hide_locked() {
        if (!ov_on) return;
        ov_on = false;
        for (int r = ov_top; r < ov_top + ov_h; r++) {
            for (int c = ov_left; c < ov_left + ov_w; c++) {
                if (r == 0 && c >= UPTIME_COL) continue;
                VGA[r * W + c] = screen[r * W + c];   // tampilkan teks yang sebenarnya
            }
        }
    }

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
        ensure_screen();

        int target = view_offset + lines;
        if (target > hist_count) target = hist_count;
        if (target < 0) target = 0;

        if (target != view_offset) {
            if (view_offset == 0) {
                overlay_hide_locked();   // tutup pop-up dulu supaya yang disimpan layar aslinya
                for (int i = 0; i < W * H; i++) {
                    snapshot[i] = screen[i];   // simpan layar aktif sebelum ditimpa
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
        u32 f = irq_save();

        for (int i = 0; i < W * H; i++) {
            u16 cell = (u16)' ' | (COLOR << 8);
            screen[i] = cell;
            if (!covered(i / W, i % W)) {
                VGA[i] = cell;
            }
        }
        screen_ready = true;

        row = 0;
        col = 0;
        view_offset = 0;
        hist_count = 0;
        hist_next = 0;
        update_cursor();

        irq_restore(f);
    }

    // Menyimpan baris paling atas ke riwayat sebelum terdorong keluar layar
    static void save_top_line() {
        for (int c = 0; c < W; c++) {
            u16 cell = screen[c];

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

            screen[i] = screen[i + W];
        }

        for (int i = W * (H - 1); i < W * H; i++) {
            screen[i] = (u16)' ' | (COLOR << 8);
        }

        repaint();
        row = H - 1;
    }

    // Menampilkan satu karakter
    void putc(char c) {
        u32 f = irq_save();
        ensure_screen();
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
                put_cell(row, col, (u16)' ' | (COLOR << 8));
            }
        }
        else {
            put_cell(row, col, (u16)(u8)c | (COLOR << 8));
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
        irq_restore(f);
    }

    // ---------- API pop-up ----------
    bool overlay_show(int top, int left, int height, int width) {
        u32 f = irq_save();
        ensure_screen();

        bool ok = view_offset == 0 &&
                  top >= 1 && left >= 0 && height > 0 && width > 0 &&
                  top + height <= H && left + width <= W;

        if (ok) {
            overlay_hide_locked();   // kalau sudah ada pop-up lama, tutup dulu
            ov_on = true;
            ov_top = top;
            ov_left = left;
            ov_h = height;
            ov_w = width;
        }

        irq_restore(f);
        return ok;
    }

    void overlay_put(int r, int c, char ch, u8 color) {
        u32 f = irq_save();

        if (ov_on && r >= 0 && c >= 0 && r < ov_h && c < ov_w) {
            VGA[(ov_top + r) * W + ov_left + c] = (u16)(u8)ch | ((u16)color << 8);
        }

        irq_restore(f);
    }

    void overlay_print(int r, int c, const char* s, u8 color) {
        for (int i = 0; s[i] != '\0'; i++) {
            overlay_put(r, c + i, s[i], color);
        }
    }

    void overlay_hide() {
        u32 f = irq_save();
        overlay_hide_locked();
        irq_restore(f);
    }

    bool overlay_visible() {
        return ov_on;
    }

}
