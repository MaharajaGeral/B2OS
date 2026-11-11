#pragma once
#pragma once
#include "kernel/types.h"

enum class ThreadState : u8 { UNUSED, NEW, READY, RUNNING, WAITING, TERMINATED };
typedef void (*ThreadEntry)(void*);

// Thread Control Block (TCB): data yang disimpan kernel untuk setiap thread
struct Thread {
    u32         id;
    char        name[16];
    ThreadState state;
    u32         esp;          // stack pointer tersimpan saat thread tidak berjalan
    u8*         stack_base;
    ThreadEntry entry;        // fungsi yang dijalankan thread
    void*       arg;
    bool        sleeping;
    u32         wake_tick;
    u32         cpu_ticks;    // total tick CPU yang dipakai
    bool        is_system;    // shell dan idle: tidak dicatat di trace
};

namespace thread {
const int MAX = 16;
const u32 STACK_SIZE = 8192;

void        init_table();
Thread*     at(int i);
Thread*     alloc(const char* name, ThreadEntry fn, void* arg, bool system);
void        set_state(Thread* t, ThreadState s);
const char* state_name(ThreadState s);
void        set_trace(bool on);
bool        trace();
}