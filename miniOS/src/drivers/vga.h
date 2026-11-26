#pragma once
#include "kernel/types.h"

namespace vga {
    void clear();
    void putc(char c);

    // Menggulung tampilan layar. lines > 0: naik (melihat riwayat), lines < 0: turun.
    // Menulis karakter baru otomatis mengembalikan tampilan ke bagian paling bawah.
    void scroll_view(int lines);

    // ---------- Pop-up (overlay) ----------
    // Kotak sementara di atas layar. Teks terminal di bawahnya tetap tercatat dan
    // muncul utuh begitu kotak ditutup. Baris 0 (jam "UP") tidak boleh ditutup.
    // overlay_show gagal (false) bila area di luar layar, menyentuh baris 0, atau layar
    // sedang dalam mode riwayat. Koordinat r, c pada overlay_put/overlay_print relatif
    // terhadap pojok kiri atas kotak.
    bool overlay_show(int top, int left, int height, int width);
    void overlay_put(int r, int c, char ch, u8 color);
    void overlay_print(int r, int c, const char* s, u8 color);   // terpotong di tepi kotak
    void overlay_hide();
    bool overlay_visible();
}
