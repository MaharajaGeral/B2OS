#include "drivers/keyboard.h"
#include "kernel/io.h"
#include "drivers/vga.h"
#include "kernel/types.h"

namespace keyboard {

static const char NORMAL[] =
    "\0\033" "1234567890-=\b\t"
    "qwertyuiop[]\n\0"
    "asdfghjkl;'`\0\\"
    "zxcvbnm,./\0*\0 ";

static const char SHIFTED[] =
    "\0\033" "!@#$%^&*()_+\b\t"
    "QWERTYUIOP{}\n\0"
    "ASDFGHJKL:\"~\0|"
    "ZXCVBNM<>?\0*\0 ";

static bool shift = false;
static bool ext = false;

void init() {
    shift = false;
    ext = false;
}

char read_char() {
    if (!(inb(0x64) & 1)) {
        return 0;
    }

    u8 sc = inb(0x60);

    if (sc == 0xE0) {
        ext = true;
        return 0;
    }

    if (ext) {
        ext = false;

        if (sc == 0x49) vga::scroll_view(10);
        if (sc == 0x51) vga::scroll_view(-10);
        if (sc == 0x48) vga::scroll_view(1);
        if (sc == 0x50) vga::scroll_view(-1);

        return 0;
    }

    if (sc == 0x2A || sc == 0x36) {
        shift = true;
        return 0;
    }

    if (sc == 0xAA || sc == 0xB6) {
        shift = false;
        return 0;
    }

    if (sc & 0x80) {
        return 0;
    }

    if (sc < sizeof(NORMAL) - 1) {
        return shift ? SHIFTED[sc] : NORMAL[sc];
    }

    return 0;
}

}