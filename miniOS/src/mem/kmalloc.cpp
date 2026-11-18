// Alokator memori dinamis: first-fit dengan pemecahan blok (split) dan penggabungan (coalesce).
//
// Heap = array statis. Tiap blok diawali header berisi ukuran, status, dan penunjuk ke blok
// berikutnya. Blok tersusun berurutan menurut alamat, sehingga blok bebas yang bersebelahan
// mudah digabung kembali saat kfree.
#include "mem/kmalloc.h"
#include "kernel/cpu.h"
#include "kernel/kernel.h"

struct Block {
    u32    size;       // ukuran data (tanpa header)
    u32    magic;      // penanda blok bebas / terpakai (untuk mendeteksi pointer salah)
    Block* next;       // blok berikutnya (urut alamat)
};

static const u32   MAGIC_FREE = 0xF4EEF4EE;
static const u32   MAGIC_USED = 0xA110C8ED;
static const usize HDR        = (sizeof(Block) + 15) & ~(usize)15;   // header dibulatkan ke 16 byte
static const usize MIN_SPLIT  = 16;                                  // sisa minimum agar blok dipecah

static u8     heap[mem::HEAP_SIZE] __attribute__((aligned(16)));
static Block* head = nullptr;
static u32    n_alloc_ok = 0, n_alloc_fail = 0, n_free_ok = 0, n_free_bad = 0;

static void ensure_init() {
    if (head) return;
    head = (Block*)heap;
    head->size  = mem::HEAP_SIZE - HDR;
    head->magic = MAGIC_FREE;
    head->next  = nullptr;
}

static inline u8* payload(Block* b) { return (u8*)b + HDR; }

void* kmalloc(usize n) {
    if (n == 0) n = 1;
    n = (n + 15) & ~(usize)15;                         // bulatkan ke kelipatan 16
    u32 f = irq_save();
    ensure_init();

    for (Block* b = head; b; b = b->next) {            // first-fit: blok bebas pertama yang cukup
        if (b->magic != MAGIC_FREE || b->size < n) continue;

        if (b->size >= n + HDR + MIN_SPLIT) {          // sisa cukup besar -> pecah jadi dua blok
            Block* rest = (Block*)(payload(b) + n);
            rest->size  = b->size - n - HDR;
            rest->magic = MAGIC_FREE;
            rest->next  = b->next;
            b->size = n;
            b->next = rest;
        }
        b->magic = MAGIC_USED;
        n_alloc_ok++;
        irq_restore(f);
        return payload(b);
    }

    n_alloc_fail++;                                    // tidak ada blok cukup: Out of Memory
    irq_restore(f);
    return nullptr;                                    // gagal dengan aman, kernel tidak crash
}

void kfree(void* p) {
    if (!p) return;
    u32 f = irq_save();
    ensure_init();

    u8* ptr = (u8*)p;
    bool in_range = ptr >= heap + HDR && ptr < heap + mem::HEAP_SIZE;
    Block* b = in_range ? (Block*)(ptr - HDR) : nullptr;
    if (!b || b->magic != MAGIC_USED) {                // pointer salah / double free -> tolak
        n_free_bad++;
        irq_restore(f);
        return;
    }

    b->magic = MAGIC_FREE;
    n_free_ok++;

    // Gabungkan blok bebas yang bersebelahan agar tidak terjadi fragmentasi.
    for (Block* c = head; c; c = c->next) {
        while (c->magic == MAGIC_FREE && c->next && c->next->magic == MAGIC_FREE) {
            c->size += HDR + c->next->size;
            c->next  = c->next->next;
        }
    }
    irq_restore(f);
}

// ---------- statistik ----------
usize mem::total() { return HEAP_SIZE; }

usize mem::free_bytes() {
    u32 f = irq_save(); ensure_init();
    usize s = 0;
    for (Block* b = head; b; b = b->next) if (b->magic == MAGIC_FREE) s += b->size;
    irq_restore(f);
    return s;
}
usize mem::used_bytes() { return HEAP_SIZE - free_bytes(); }

usize mem::largest_free() {
    u32 f = irq_save(); ensure_init();
    usize m = 0;
    for (Block* b = head; b; b = b->next) if (b->magic == MAGIC_FREE && b->size > m) m = b->size;
    irq_restore(f);
    return m;
}
u32 mem::block_count() {
    u32 f = irq_save(); ensure_init();
    u32 n = 0;
    for (Block* b = head; b; b = b->next) n++;
    irq_restore(f);
    return n;
}
u32 mem::used_blocks() {
    u32 f = irq_save(); ensure_init();
    u32 n = 0;
    for (Block* b = head; b; b = b->next) if (b->magic == MAGIC_USED) n++;
    irq_restore(f);
    return n;
}
u32 mem::alloc_ok()   { return n_alloc_ok; }
u32 mem::alloc_fail() { return n_alloc_fail; }
u32 mem::free_ok()    { return n_free_ok; }
u32 mem::free_bad()   { return n_free_bad; }

void mem_print_stats() {
    u32 f = irq_save();
    kernel_print("Heap total   : "); kernel_print_dec(mem::total());        kernel_print(" byte\n");
    kernel_print("Terpakai     : "); kernel_print_dec(mem::used_bytes());   kernel_print(" byte ("); kernel_print_dec(mem::used_blocks()); kernel_print(" blok)\n");
    kernel_print("Tersedia     : "); kernel_print_dec(mem::free_bytes());   kernel_print(" byte (blok bebas terbesar "); kernel_print_dec(mem::largest_free()); kernel_print(" byte)\n");
    kernel_print("Jumlah blok  : "); kernel_print_dec(mem::block_count());  kernel_print("\n");
    kernel_print("kmalloc      : "); kernel_print_dec(mem::alloc_ok());     kernel_print(" berhasil, "); kernel_print_dec(mem::alloc_fail()); kernel_print(" gagal (Out of Memory)\n");
    kernel_print("kfree        : "); kernel_print_dec(mem::free_ok());      kernel_print(" valid, "); kernel_print_dec(mem::free_bad()); kernel_print(" ditolak (pointer tidak valid)\n");
    irq_restore(f);
}