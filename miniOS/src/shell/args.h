#pragma once

// Pengurai argumen untuk perintah shell.
// Contoh: "beras 50 A1"  ->  token[0]="beras", token[1]="50", token[2]="A1"

const int ARGS_MAX_TOKENS = 4;    // maksimal jumlah token
const int ARGS_TOKEN_SIZE = 32;   // panjang tiap token, termasuk '\0' (maks 31 karakter)

// Memecah 'text' menjadi token yang dipisahkan spasi/tab.
// Hasil: jumlah token (0..4), atau -1 bila token terlalu banyak / terlalu panjang.
int args_split(const char* text, char tokens[ARGS_MAX_TOKENS][ARGS_TOKEN_SIZE]);