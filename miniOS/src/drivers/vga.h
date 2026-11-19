#pragma once

namespace vga {
    void clear();
    void putc(char c);

    // Menggulung tampilan layar. lines > 0: naik (melihat riwayat), lines < 0: turun.
    // Menulis karakter baru otomatis mengembalikan tampilan ke bagian paling bawah.
    void scroll_view(int lines);
}