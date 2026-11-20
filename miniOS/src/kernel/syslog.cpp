// syslog: log kernel opsional. Dipakai tiap modul supaya fitur OS terlihat saat aplikasi berjalan.
#include "kernel/syslog.h"
#include "kernel/kernel.h"
#include "kernel/cpu.h"
#include "proc/thread.h"

static bool is_on = false;

void syslog::enable(bool on) {
    is_on = on;
    thread::set_trace(on);        // perubahan status thread ikut tercetak
}

bool syslog::enabled() { return is_on; }

// Cetak "[TAG   ] " dengan TAG dipadatkan/dipotong menjadi 6 karakter.
static void print_tag(const char* tag) {
    kernel_print("[");
    int n = 0;
    while (tag[n] && n < 6) {
        char c[2] = { tag[n], 0 };
        kernel_print(c);
        n++;
    }
    while (n < 6) { kernel_print(" "); n++; }
    kernel_print("] ");
}

void syslog::log(const char* tag, const char* msg) {
    if (!is_on) return;
    u32 f = irq_save();                      // satu baris utuh, tidak terpotong thread lain
    print_tag(tag);
    kernel_print(msg);
    kernel_print("\n");
    irq_restore(f);
}

void syslog::log_s(const char* tag, const char* msg, const char* text) {
    if (!is_on) return;
    u32 f = irq_save();
    print_tag(tag);
    kernel_print(msg);
    kernel_print(" ");
    kernel_print(text);
    kernel_print("\n");
    irq_restore(f);
}

void syslog::log_n(const char* tag, const char* msg, u32 n) {
    if (!is_on) return;
    u32 f = irq_save();
    print_tag(tag);
    kernel_print(msg);
    kernel_print(" ");
    kernel_print_dec(n);
    kernel_print("\n");
    irq_restore(f);
}

void syslog::log_who(const char* tag, const char* who, const char* msg, const char* text) {
    if (!is_on) return;
    u32 f = irq_save();
    print_tag(tag);
    kernel_print(who);
    kernel_print(" ");
    kernel_print(msg);
    kernel_print(" ");
    kernel_print(text);
    kernel_print("\n");
    irq_restore(f);
}