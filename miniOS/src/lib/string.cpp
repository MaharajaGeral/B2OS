
#include "lib/string.h"

// Mengisi memori dengan nilai tertentu.
extern "C" void* memset(void* dst, int v, usize n) {
    unsigned char* p = static_cast<unsigned char*>(dst);

    for (usize i = 0; i < n; i++) {
        p[i] = static_cast<unsigned char>(v);
    }

    return dst;
}

// Menyalin data dari sumber ke tujuan.
extern "C" void* memcpy(void* dst, const void* src, usize n) {
    unsigned char* d = static_cast<unsigned char*>(dst);
    const unsigned char* s =
        static_cast<const unsigned char*>(src);

    for (usize i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dst;
}

// Membandingkan dua blok memori.
extern "C" int memcmp(const void* a, const void* b, usize n) {
    const unsigned char* p =
        static_cast<const unsigned char*>(a);
    const unsigned char* q =
        static_cast<const unsigned char*>(b);

    for (usize i = 0; i < n; i++) {
        if (p[i] != q[i]) {
            return (p[i] < q[i]) ? -1 : 1;
        }
    }

    return 0;
}

// Menghitung panjang string tanpa karakter null terminator.
extern "C" usize strlen(const char* s) {
    usize length = 0;

    while (s[length] != '\0') {
        length++;
    }

    return length;
}

// Membandingkan dua string.
extern "C" int strcmp(const char* a, const char* b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }

    unsigned char ca = static_cast<unsigned char>(*a);
    unsigned char cb = static_cast<unsigned char>(*b);

    if (ca < cb) {
        return -1;
    }

    if (ca > cb) {
        return 1;
    }

    return 0;
}

// Membandingkan string maksimal sebanyak n karakter.
extern "C" int strncmp(
    const char* a,
    const char* b,
    usize n
) {
    for (usize i = 0; i < n; i++) {
        unsigned char ca = static_cast<unsigned char>(a[i]);
        unsigned char cb = static_cast<unsigned char>(b[i]);

        if (ca != cb) {
            return (ca < cb) ? -1 : 1;
        }

        if (ca == '\0') {
            return 0;
        }
    }

    return 0;
}

// Menyalin seluruh string sumber ke tujuan.
extern "C" char* strcpy(char* dst, const char* src) {
    char* result = dst;

    while (*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }

    *dst = '\0';

    return result;
}

// Menyalin string dengan batas maksimal n karakter.
extern "C" char* strncpy(
    char* dst,
    const char* src,
    usize n
) {
    if (n == 0) {
        return dst;
    }

    usize i = 0;

    while (i < n - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    // Mengisi sisa buffer dengan null terminator.
    while (i < n) {
        dst[i] = '\0';
        i++;
    }

    return dst;
}

// Mengubah string angka desimal menjadi u32.
bool parse_u32(const char* s, u32* out) {
    if (s == nullptr || out == nullptr || *s == '\0') {
        return false;
    }

    u32 result = 0;

    while (*s != '\0') {
        if (*s < '0' || *s > '9') {
            return false;
        }

        u32 digit = static_cast<u32>(*s - '0');

        // Memeriksa kemungkinan overflow u32.
        if (result > (0xFFFFFFFFu - digit) / 10u) {
            return false;
        }

        result = result * 10u + digit;
        s++;
    }

    *out = result;

    return true;
}
