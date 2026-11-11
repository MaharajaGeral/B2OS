// Demo kriteria 4: sinkronisasi. Membuktikan race condition dan memperbaikinya.
//  demo_race : dua thread menaikkan satu counter bersama, tanpa mutex lalu dengan mutex.
//  demo_sem  : semaphore membatasi jumlah thread yang boleh masuk bersamaan.
#include "proc/demo.h"
#include "proc/scheduler.h"
#include "sync/mutex.h"
#include "sync/semaphore.h"
#include "kernel/timer.h"
#include "kernel/kernel.h"
#include "kernel/cpu.h"

// Cetak satu baris secara utuh supaya tidak tercampur thread lain
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

static void wait_all_user_threads() {
    while (sched::user_threads_alive() > 0) sched::yield();
}

// ---------------------------------------------------------------
// demo_race: shared counter
// ---------------------------------------------------------------
static const u32 INCREMENTS = 20;          // kenaikan per thread -> total seharusnya 2 x 20 = 40

static volatile u32 shared_counter = 0;
static Mutex counter_lock;
static bool use_lock = false;

// Menunggu sampai tick berikutnya (maksimal 10 ms). Di sinilah thread bisa dipotong timer.
static void wait_next_tick() {
    u32 s = timer::ticks();
    while (timer::ticks() == s) {}
}

static void racer(void*) {
    for (u32 i = 0; i < INCREMENTS; i++) {
        if (use_lock) mutex_lock(&counter_lock);

        u32 tmp = shared_counter;          // 1. BACA nilai counter
        wait_next_tick();                  // 2. "bekerja" - thread lain bisa masuk di sini
        shared_counter = tmp + 1;          // 3. TULIS (bisa menimpa kenaikan thread lain)

        if (use_lock) mutex_unlock(&counter_lock);
    }
}

static u32 run_race(bool with_lock) {
    use_lock = with_lock;
    shared_counter = 0;
    sched::create("racer-1", racer, nullptr);
    sched::create("racer-2", racer, nullptr);
    wait_all_user_threads();
    return shared_counter;
}

void demo_race() {
    const u32 expected = 2 * INCREMENTS;
    mutex_init(&counter_lock, "counter");

    kernel_print("--- demo_race: 2 thread, masing-masing menaikkan counter ");
    kernel_print_dec(INCREMENTS);
    kernel_print(" kali ---\n");
    kernel_print("Nilai counter seharusnya: ");
    kernel_print_dec(expected);
    kernel_print("\n");

    kernel_print("[1] TANPA mutex ... (tunggu sebentar)\n");
    u32 r1 = run_race(false);
    say("    hasil = ", r1, "");
    kernel_print(r1 == expected ? "  -> kebetulan benar, ulangi lagi\n"
                                : "  -> SALAH, race condition terjadi\n");

    kernel_print("[2] DENGAN mutex ... (tunggu sebentar)\n");
    u32 r2 = run_race(true);
    say("    hasil = ", r2, "");
    kernel_print(r2 == expected ? "  -> BENAR, data konsisten\n"
                                : "  -> masih salah, cek mutex\n");

    kernel_print("--- demo_race selesai ---\n");
}

// ---------------------------------------------------------------
// demo_sem: semaphore dengan 2 slot, 4 thread
// ---------------------------------------------------------------
static const int SLOTS = 2;
static Semaphore slots;
static volatile int inside = 0;
static volatile int max_inside = 0;

static void sem_worker(void* arg) {
    u32 id = (u32)(unsigned long)arg;

    say("[S", id, "] menunggu slot\n");
    sem_wait(&slots);                      // slot habis -> thread berstatus WAITING

    u32 f = irq_save();
    inside = inside + 1;
    if (inside > max_inside) max_inside = inside;
    int now = inside;
    irq_restore(f);
    say2("[S", id, "] MASUK,  jumlah di dalam = ", (u32)now, "\n");

    sched::sleep_ms(300);                  // memakai slot selama 300 ms

    f = irq_save();
    inside = inside - 1;
    irq_restore(f);
    say("[S", id, "] KELUAR\n");

    sem_signal(&slots);                    // kembalikan slot, bangunkan yang menunggu
}

void demo_sem() {
    bool old = thread::trace();
    thread::set_trace(true);               // tampilkan perubahan status (WAITING terlihat)

    sem_init(&slots, SLOTS);
    inside = 0;
    max_inside = 0;

    kernel_print("--- demo_sem: semaphore ");
    kernel_print_dec((u32)SLOTS);
    kernel_print(" slot, 4 thread ---\n");

    for (u32 i = 1; i <= 4; i++) {
        char name[8] = { 's', 'e', 'm', '-', (char)('0' + i), 0, 0, 0 };
        sched::create(name, sem_worker, (void*)(unsigned long)i);
    }
    wait_all_user_threads();

    thread::set_trace(old);

    say2("Maksimum thread di dalam bersamaan = ", (u32)max_inside,
         ", batas semaphore = ", (u32)SLOTS, "\n");
    kernel_print(max_inside <= SLOTS ? "-> OK, semaphore membatasi dengan benar\n"
                                     : "-> SALAH, batas terlampaui\n");
    kernel_print("--- demo_sem selesai ---\n");
}