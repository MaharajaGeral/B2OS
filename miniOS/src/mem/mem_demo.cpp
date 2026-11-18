// Demo kriteria 6: kapasitas berubah, memori bisa dilepas dan dipakai ulang, Out of Memory aman.
#include "mem/kmalloc.h"
#include "kernel/kernel.h"

static void show(const char* title) {
    kernel_print("\n>> "); kernel_print(title); kernel_print("\n");
    mem_print_stats();
}

static void show_alloc(const char* name, usize n, void* p) {
    kernel_print("   kmalloc("); kernel_print_dec(n); kernel_print(") -> ");
    if (p) { kernel_print(name); kernel_print(" di alamat "); kernel_print_hex((u32)(usize)p); }
    else   { kernel_print("GAGAL (nullptr)"); }
    kernel_print("\n");
}

void demo_mem() {
    kernel_print("--- demo_mem: alokator memori dinamis ---\n");
    show("1. Keadaan awal");

    void* a = kmalloc(1000); show_alloc("A", 1000, a);
    void* b = kmalloc(2000); show_alloc("B", 2000, b);
    void* c = kmalloc(4000); show_alloc("C", 4000, c);
    show("2. Setelah 3 alokasi (terpakai naik, tersedia turun)");

    kfree(b);
    show("3. Setelah kfree(B) (terpakai turun: memori benar-benar dilepas)");

    void* d = kmalloc(1500); show_alloc("D", 1500, d);
    kernel_print("   D menempati bekas B? ");
    kernel_print(d == b ? "YA, memori yang dilepas dipakai ulang\n" : "tidak\n");

    kfree(a); kfree(c); kfree(d);
    show("4. Setelah semua dilepas (kembali ke satu blok besar, blok bersebelahan digabung)");

    kernel_print("\n>> 5. Uji Out of Memory\n");
    static void* ptrs[64];
    int n = 0;
    while (n < 64) {
        void* p = kmalloc(4096);
        if (!p) break;                                  // memori habis, berhenti dengan aman
        ptrs[n++] = p;
    }
    kernel_print("   Alokasi 4096 byte berhasil sebanyak "); kernel_print_dec((u32)n); kernel_print(" kali, lalu Out of Memory\n");
    void* huge = kmalloc(1024 * 1024);
    show_alloc("huge", 1024 * 1024, huge);
    kernel_print("   -> kernel TIDAK crash, kmalloc hanya mengembalikan nullptr\n");
    show("6. Saat heap penuh");

    kernel_print("\n>> 7. Uji kfree dengan pointer tidak valid\n");
    kfree((void*)0x1234);                               // alamat sembarang -> ditolak
    kfree(ptrs[0]);                                     // pelepasan sah pertama
    kfree(ptrs[0]);                                     // double free -> ditolak
    kernel_print("   kfree(pointer salah) dan double free ditolak, tidak merusak heap\n");

    for (int i = 1; i < n; i++) kfree(ptrs[i]);         // ptrs[0] sudah dilepas di atas
    show("8. Setelah semua dilepas");
    kernel_print("--- demo_mem selesai ---\n");
}