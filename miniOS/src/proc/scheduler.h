#pragma once
#pragma once
#include "proc/thread.h"

namespace sched {
const u32 QUANTUM = 5;            // jatah per giliran: 5 tick = 50 ms

void    init();                   // thread 0 = shell, thread 1 = idle
Thread* create(const char* name, ThreadEntry fn, void* arg);
Thread* current();
void    tick();                   // dipanggil timer setiap interrupt
void    yield();                  // serahkan CPU secara sukarela
void    sleep_ms(u32 ms);         // tidur (status WAITING)
void    exit();                   // akhiri thread (status TERMINATED)
void    print_threads();
u32     user_threads_alive();
}