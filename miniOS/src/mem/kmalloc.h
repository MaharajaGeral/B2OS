#pragma once
#include "kernel/types.h"

// Alokator memori dinamis kernel (heap statis 64 KB, algoritma first-fit).
void* kmalloc(usize n);     // minta n byte; mengembalikan nullptr jika memori habis (Out of Memory)
void  kfree(void* p);       // lepaskan memori; pointer tidak valid diabaikan dengan aman

namespace mem {
const usize HEAP_SIZE = 64 * 1024;

usize total();              // ukuran heap
usize free_bytes();         // total memori bebas (di semua blok bebas)
usize used_bytes();         // memori terpakai (termasuk header blok)
usize largest_free();       // blok bebas terbesar
u32   block_count();        // jumlah blok (bebas + terpakai)
u32   used_blocks();        // jumlah blok terpakai
u32   alloc_ok();           // jumlah kmalloc yang berhasil
u32   alloc_fail();         // jumlah kmalloc yang gagal (Out of Memory)
u32   free_ok();            // jumlah kfree yang valid
u32   free_bad();           // jumlah kfree dengan pointer tidak valid (ditolak)
}

void mem_print_stats();     // dipakai perintah 'memstat'
void demo_mem();            // dipakai perintah 'memtest'