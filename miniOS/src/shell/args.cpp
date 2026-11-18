#include "shell/args.h"

static bool is_space(char c) {
    return c == ' ' || c == '\t';
}

int args_split(const char* text, char tokens[ARGS_MAX_TOKENS][ARGS_TOKEN_SIZE]) {
    // Kosongkan semua token dengan loop manual (tanpa memset)
    for (int i = 0; i < ARGS_MAX_TOKENS; i++) {
        tokens[i][0] = '\0';
    }

    if (!text) {
        return 0;
    }

    int count = 0;

    while (*text != '\0') {
        // Lewati spasi di antara token
        while (is_space(*text)) {
            text++;
        }

        if (*text == '\0') {
            break;
        }

        // Token ke-5 dan seterusnya tidak diterima
        if (count >= ARGS_MAX_TOKENS) {
            return -1;
        }

        // Salin satu token
        int length = 0;

        while (*text != '\0' && !is_space(*text)) {
            if (length >= ARGS_TOKEN_SIZE - 1) {
                return -1;   // token terlalu panjang
            }

            tokens[count][length++] = *text++;
        }

        tokens[count][length] = '\0';
        count++;
    }

    return count;
}