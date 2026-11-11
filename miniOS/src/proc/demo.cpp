// Demo untuk kriteria 2 (thread) dan 3 (scheduler preemptive).
#include "proc/demo.h"
#include "proc/scheduler.h"
#include "kernel/timer.h"
#include "kernel/kernel.h"
#include "kernel/cpu.h"

// Cetak satu baris secara utuh supaya tidak terpotong thread lain
static void say(const char* a, u32 n, const char* b) {
    u32 f = irq_save();
    kernel_print(a); kernel_print_dec(n); kernel_print(b);
    irq_restore(f);
}
static void say2(const char* a, u32 n, const char* b, u32 m, const char* c) {
    u32 f = irq_save();
    kernel_print(a); kernel_print_dec(n); kernel_print(b); kernel_print_dec(m); kernel_print(c);
    irq_restore(f);
}
static void report(const char* name, const char* what, u32 iter) {
    u32 f = irq_save();
    kernel_print("["); kernel_print(name); kernel_print("] "); kernel_print(what);
    kernel_print_dec(iter); kernel_print(" (tick="); kernel_print_dec(timer::ticks()); kernel_print(")\n");
    irq_restore(f);
}

static void spin_ticks(u32 n) {            // menunggu TANPA menyerahkan CPU (busy-wait)
    u32 s = timer::ticks();
    while (timer::ticks() - s < n) {}
}
static void wait_user_threads() {
    while (sched::user_threads_alive() > 0) sched::yield();
}

// ---------- demo_thread ----------
static void worker_sleeper(void*) {        // tugas A: bekerja lalu tidur (status WAITING)
    for (u32 i = 1; i <= 4; i++) {
        say("[worker-A] langkah ", i, "/4, tidur 300 ms\n");
        sched::sleep_ms(300);
    }
    kernel_print("[worker-A] selesai\n");
}
static void worker_compute(void*) {        // tugas B: menghitung jumlah (CPU-bound)
    u32 sum = 0;
    for (u32 step = 1; step <= 3; step++) {
        for (volatile u32 i = 1; i <= 200000; i++) sum += i;
        say2("[worker-B] langkah ", step, "/3, sum sementara=", sum, "\n");
        spin_ticks(10);
    }
    say("[worker-B] selesai, sum akhir=", sum, "\n");
}

void demo_thread() {
    bool old = thread::trace();
    thread::set_trace(true);
    kernel_print("--- demo_thread: membuat 2 thread ---\n");
    sched::create("worker-A", worker_sleeper, nullptr);
    sched::create("worker-B", worker_compute, nullptr);
    wait_user_threads();
    kernel_print("--- demo_thread selesai. Ketik 'ps' untuk melihat status akhir ---\n");
    thread::set_trace(old);
}

// ---------- demo_sched ----------
static void cpu_worker(void* arg) {        // tidak pernah yield: giliran diatur timer
    const char* n = (const char*)arg;
    for (u32 i = 1; i <= 4; i++) {
        report(n, "mulai   iterasi ", i);
        spin_ticks(12);                    // lebih lama dari jatah (5 tick) -> pasti di-preempt
        report(n, "selesai iterasi ", i);
    }
}

void demo_sched() {
    kernel_print("--- demo_sched: A dan B tidak pernah yield, bergantian karena timer ---\n");
    sched::create("A", cpu_worker, (void*)"A");
    sched::create("B", cpu_worker, (void*)"B");
    wait_user_threads();
    kernel_print("--- demo_sched selesai ---\n");
}

void show_threads() { sched::print_threads(); }
void trace_enable(bool on) { thread::set_trace(on); }
bool trace_enabled() { return thread::trace(); }