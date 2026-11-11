// Scheduler Round Robin preemptive.
#include "proc/scheduler.h"
#include "kernel/cpu.h"
#include "kernel/timer.h"
#include "kernel/kernel.h"

extern "C" void switch_context(u32* old_esp, u32 new_esp);

static Thread* cur    = nullptr;
static Thread* idle_t = nullptr;
static u32 quantum_left = sched::QUANTUM;

static void idle_fn(void*) { for (;;) asm volatile("hlt"); }

void sched::init() {
    thread::init_table();
    Thread* boot = thread::at(0);          // konteks kernel_main menjadi thread "shell"
    const char* nm = "shell";
    int i = 0;
    while (nm[i]) { boot->name[i] = nm[i]; i++; }
    boot->name[i] = 0;
    boot->is_system = true;
    boot->state = ThreadState::RUNNING;
    cur = boot;
    idle_t = thread::alloc("idle", idle_fn, nullptr, true);
}

Thread* sched::create(const char* name, ThreadEntry fn, void* arg) {
    return thread::alloc(name, fn, arg, false);
}
Thread* sched::current() { return cur; }

// Round Robin: cari thread READY berikutnya setelah thread yang sedang jalan.
static Thread* pick_next(Thread* old) {
    int base = (int)old->id;
    for (int i = 1; i <= thread::MAX; i++) {
        Thread* t = thread::at((base + i) % thread::MAX);
        if (t == idle_t) continue;
        if (t->state == ThreadState::READY) return t;
        if (t == old && t->state == ThreadState::RUNNING) return t;
    }
    return idle_t;                         // tidak ada yang siap -> idle
}

// Pindah ke thread berikutnya. Harus dipanggil dengan interrupt nonaktif.
static void schedule() {
    Thread* old  = cur;
    Thread* next = pick_next(old);
    quantum_left = sched::QUANTUM;
    if (next == old) return;
    if (old->state == ThreadState::RUNNING) thread::set_state(old, ThreadState::READY);
    thread::set_state(next, ThreadState::RUNNING);
    cur = next;
    switch_context(&old->esp, next->esp);  // context switch sungguhan
}

// Dipanggil timer tiap 10 ms. Di sinilah preemption terjadi.
void sched::tick() {
    u32 now = timer::ticks();
    for (int i = 0; i < thread::MAX; i++) {            // bangunkan thread yang tidurnya selesai
        Thread* t = thread::at(i);
        if (t->state == ThreadState::WAITING && t->sleeping && (i32)(now - t->wake_tick) >= 0) {
            t->sleeping = false;
            thread::set_state(t, ThreadState::READY);
        }
    }
    cur->cpu_ticks++;
    if (quantum_left > 0) quantum_left--;
    if (quantum_left == 0) schedule();                 // jatah habis -> thread dipaksa gantian
}

void sched::yield() {
    u32 f = irq_save();
    schedule();
    irq_restore(f);
}

void sched::sleep_ms(u32 ms) {
    u32 f = irq_save();
    cur->wake_tick = timer::ticks() + (ms + 9) / 10;
    cur->sleeping = true;
    thread::set_state(cur, ThreadState::WAITING);
    schedule();
    irq_restore(f);
}

void sched::exit() {
    irq_save();
    thread::set_state(cur, ThreadState::TERMINATED);
    schedule();
    for (;;) asm volatile("hlt");                      // tidak pernah tercapai
}

// ---------- dasar untuk mutex dan semaphore ----------

// Thread sekarang menunggu sesuatu (bukan tidur berbatas waktu), lalu CPU diberikan ke thread lain.
// Thread ini baru jalan lagi setelah ada yang memanggil unblock() untuknya.
void sched::block() {
    cur->sleeping = false;
    thread::set_state(cur, ThreadState::WAITING);
    schedule();
}

void sched::unblock(Thread* t) {
    if (t->state == ThreadState::WAITING && !t->sleeping)
        thread::set_state(t, ThreadState::READY);
}

void sched::kill(Thread* t) {
    u32 f = irq_save();
    if (t && t != cur && t->state != ThreadState::UNUSED && t->state != ThreadState::TERMINATED) {
        t->sleeping = false;
        thread::set_state(t, ThreadState::TERMINATED);
    }
    irq_restore(f);
}

static void print_padded(const char* s, int width) {
    int n = 0;
    while (s[n]) n++;
    kernel_print(s);
    while (n++ < width) kernel_print(" ");
}

void sched::print_threads() {
    u32 f = irq_save();
    kernel_print("ID  NAMA          STATE       CPU(tick)\n");
    for (int i = 0; i < thread::MAX; i++) {
        Thread* t = thread::at(i);
        if (t->state == ThreadState::UNUSED) continue;
        kernel_print_dec(t->id);
        kernel_print("   ");
        print_padded(t->name, 14);
        print_padded(thread::state_name(t->state), 12);
        kernel_print_dec(t->cpu_ticks);
        kernel_print("\n");
    }
    irq_restore(f);
}

u32 sched::user_threads_alive() {
    u32 n = 0;
    for (int i = 0; i < thread::MAX; i++) {
        Thread* t = thread::at(i);
        if (!t->is_system && t->state != ThreadState::UNUSED && t->state != ThreadState::TERMINATED) n++;
    }
    return n;
}