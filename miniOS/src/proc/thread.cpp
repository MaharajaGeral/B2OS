// TCB + status thread New/Ready/Running/Waiting/Terminated (kriteria 2)
// Pengelolaan thread: tabel TCB, stack, dan status thread.
#include "proc/thread.h"
#include "proc/scheduler.h"
#include "kernel/cpu.h"
#include "kernel/kernel.h"

static Thread table[thread::MAX];
static u8 stacks[thread::MAX][thread::STACK_SIZE] __attribute__((aligned(16)));
static bool trace_on = false;

void thread::init_table() {
    for (int i = 0; i < MAX; i++) {
        table[i].id = i;
        table[i].state = ThreadState::UNUSED;
        table[i].name[0] = 0;
    }
}

Thread* thread::at(int i) { return &table[i]; }
void thread::set_trace(bool on) { trace_on = on; }
bool thread::trace() { return trace_on; }

const char* thread::state_name(ThreadState s) {
    switch (s) {
    case ThreadState::UNUSED:     return "UNUSED";
    case ThreadState::NEW:        return "NEW";
    case ThreadState::READY:      return "READY";
    case ThreadState::RUNNING:    return "RUNNING";
    case ThreadState::WAITING:    return "WAITING";
    case ThreadState::TERMINATED: return "TERMINATED";
    }
    return "?";
}

// Ubah status thread. Kalau trace aktif, perubahan dicetak ke layar.
void thread::set_state(Thread* t, ThreadState s) {
    if (trace_on && !t->is_system) {
        u32 f = irq_save();
        kernel_print("[sched] T");
        kernel_print_dec(t->id);
        kernel_print(" ");
        kernel_print(t->name);
        kernel_print(": ");
        kernel_print(state_name(t->state));
        kernel_print(" -> ");
        kernel_print(state_name(s));
        kernel_print("\n");
        irq_restore(f);
    }
    t->state = s;
}

// Titik awal semua thread baru: aktifkan interrupt, jalankan fungsinya, lalu keluar.
static void trampoline() {
    asm volatile("sti");
    Thread* t = sched::current();
    t->entry(t->arg);
    sched::exit();
}

Thread* thread::alloc(const char* name, ThreadEntry fn, void* arg, bool system) {
    u32 f = irq_save();
    Thread* t = nullptr;
    for (int i = 0; i < MAX && !t; i++)
        if (table[i].state == ThreadState::UNUSED) t = &table[i];
    for (int i = 0; i < MAX && !t; i++)
        if (table[i].state == ThreadState::TERMINATED) t = &table[i];
    if (!t) { irq_restore(f); return nullptr; }

    t->state = ThreadState::UNUSED;
    int n = 0;
    while (name[n] && n < 15) { t->name[n] = name[n]; n++; }
    t->name[n] = 0;
    t->entry = fn;
    t->arg = arg;
    t->sleeping = false;
    t->wake_tick = 0;
    t->cpu_ticks = 0;
    t->is_system = system;
    t->stack_base = stacks[t->id];

    // Siapkan stack supaya switch_context() "kembali" ke trampoline saat pertama kali dijalankan.
    u32* sp = (u32*)(t->stack_base + STACK_SIZE);
    *--sp = 0;                    // alamat return palsu untuk trampoline
    *--sp = (u32)trampoline;      // instruksi 'ret' di switch_context menuju sini
    *--sp = 0;                    // ebp
    *--sp = 0;                    // ebx
    *--sp = 0;                    // esi
    *--sp = 0;                    // edi
    t->esp = (u32)sp;

    set_state(t, ThreadState::NEW);
    set_state(t, ThreadState::READY);
    irq_restore(f);
    return t;
}