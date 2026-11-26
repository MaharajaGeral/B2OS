// Demo untuk kriteria 2 (thread) dan 3 (scheduler preemptive).
#include "proc/demo.h"
#include "proc/scheduler.h"
#include "kernel/timer.h"
#include "kernel/kernel.h"
#include "kernel/cpu.h"
#include "sync/mutex.h"

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


// ---------- demo_deadlock ----------

// Dua mutex yang digunakan dalam simulasi.
static Mutex deadlock_lock_a;
static Mutex deadlock_lock_b;

// Penanda bahwa masing-masing thread sudah memegang mutex pertama.
static volatile bool deadlock_a_ready = false;
static volatile bool deadlock_b_ready = false;

// Mencetak pesan secara utuh agar tidak bercampur dengan output thread lain.
static void deadlock_print(const char* name, const char* message) {
    u32 f = irq_save();
    kernel_print("[");
    kernel_print(name);
    kernel_print("] ");
    kernel_print(message);
    irq_restore(f);
}

// Thread A memegang Lock A, kemudian meminta Lock B.
static void deadlock_worker_a(void*) {
    mutex_lock(&deadlock_lock_a);

    deadlock_print("THREAD A", "Acquired Lock A\n");
    deadlock_a_ready = true;

    // Menunggu sampai Thread B memegang Lock B.
    while (!deadlock_b_ready) {
        sched::yield();
    }

    deadlock_print("THREAD A", "Requesting Lock B...\n");
    mutex_lock(&deadlock_lock_b);

    // Tidak akan tercapai dalam simulasi deadlock.
    mutex_unlock(&deadlock_lock_b);
    mutex_unlock(&deadlock_lock_a);
}

// Thread B memegang Lock B, kemudian meminta Lock A.
static void deadlock_worker_b(void*) {
    mutex_lock(&deadlock_lock_b);

    deadlock_print("THREAD B", "Acquired Lock B\n");
    deadlock_b_ready = true;

    // Menunggu sampai Thread A memegang Lock A.
    while (!deadlock_a_ready) {
        sched::yield();
    }

    deadlock_print("THREAD B", "Requesting Lock A...\n");
    mutex_lock(&deadlock_lock_a);

    // Tidak akan tercapai dalam simulasi deadlock.
    mutex_unlock(&deadlock_lock_a);
    mutex_unlock(&deadlock_lock_b);
}

// Kedua thread menggunakan urutan lock yang sama:
// Lock A -> Lock B.
static volatile u32 lock_order_completed = 0;

static void lock_order_worker(void* arg) {
    const char* name = (const char*)arg;

    mutex_lock(&deadlock_lock_a);
    deadlock_print(name, "Acquired Lock A\n");

    mutex_lock(&deadlock_lock_b);
    deadlock_print(name, "Acquired Lock B\n");

    deadlock_print(name, "Working...\n");

    lock_order_completed++;

    mutex_unlock(&deadlock_lock_b);
    mutex_unlock(&deadlock_lock_a);

    deadlock_print(name, "Finished successfully\n");
}

void demo_deadlock() {
    bool old_trace = thread::trace();
    thread::set_trace(true);

    kernel_print("\n========== DEADLOCK DEMO ==========\n");

    // ---------------------------------
    // TEST 1: DEADLOCK SIMULATION
    // ---------------------------------

    kernel_print("\n[1] DEADLOCK SIMULATION\n");

    mutex_init(&deadlock_lock_a, "Lock A");
    mutex_init(&deadlock_lock_b, "Lock B");

    deadlock_a_ready = false;
    deadlock_b_ready = false;

    Thread* a = sched::create(
        "deadlock-A",
        deadlock_worker_a,
        nullptr
    );

    Thread* b = sched::create(
        "deadlock-B",
        deadlock_worker_b,
        nullptr
    );

    if (!a || !b) {
        kernel_print("[ERROR] Failed to create deadlock threads.\n");

        if (a) sched::kill(a);
        if (b) sched::kill(b);

        // Thread yang dihentikan tidak boleh melanjutkan akses mutex.
        mutex_init(&deadlock_lock_a, "Lock A");
        mutex_init(&deadlock_lock_b, "Lock B");

        thread::set_trace(old_trace);
        return;
    }

    // Tunggu sampai kedua thread benar-benar WAITING.
    u32 start = timer::ticks();

    while (!(a->state == ThreadState::WAITING &&
             b->state == ThreadState::WAITING)) {

        if (timer::ticks() - start >= 100) {
            break;
        }

        sched::yield();
    }

    bool detected =
        a->state == ThreadState::WAITING &&
        b->state == ThreadState::WAITING;

    if (detected) {
        kernel_print("\n[!] DEADLOCK DETECTED!\n");
        kernel_print("Thread A holds Lock A and waits for Lock B.\n");
        kernel_print("Thread B holds Lock B and waits for Lock A.\n");
        kernel_print("Circular wait confirmed.\n");
    } else {
        kernel_print("\n[ERROR] Deadlock simulation timed out.\n");
    }

    // Hentikan kedua thread yang terjebak.
    sched::kill(a);
    sched::kill(b);

    // Bersihkan mutex setelah kedua thread dihentikan.
    // Jangan memanggil mutex_unlock dari thread yang bukan owner.
    mutex_init(&deadlock_lock_a, "Lock A");
    mutex_init(&deadlock_lock_b, "Lock B");

    // ---------------------------------
    // TEST 2: LOCK ORDERING
    // ---------------------------------

    kernel_print("\n[2] LOCK ORDERING SOLUTION\n");

    lock_order_completed = 0;

    mutex_init(&deadlock_lock_a, "Lock A");
    mutex_init(&deadlock_lock_b, "Lock B");

    Thread* ordered_a = sched::create(
        "order-A",
        lock_order_worker,
        (void*)"ORDER A"
    );

    Thread* ordered_b = sched::create(
        "order-B",
        lock_order_worker,
        (void*)"ORDER B"
    );

    if (ordered_a) sched::join(ordered_a);
    if (ordered_b) sched::join(ordered_b);

    if (lock_order_completed == 2) {
        kernel_print("\n[+] LOCK ORDERING SUCCESS!\n");
        kernel_print("Both threads completed without deadlock.\n");
    } else {
        kernel_print("\n[ERROR] Lock ordering test failed.\n");
    }

    kernel_print("\n========== DEMO FINISHED ==========\n\n");

    thread::set_trace(old_trace);
}
