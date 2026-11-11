#pragma once
#include "proc/thread.h"

// Semaphore berhitung: count = jumlah "slot" yang masih tersedia.
struct Semaphore {
    int     count;
    Thread* waiters[thread::MAX];
    int     head;
    int     size;
};

void sem_init(Semaphore* s, int initial);
void sem_wait(Semaphore* s);     // ambil satu slot; kalau habis -> WAITING
void sem_signal(Semaphore* s);   // kembalikan satu slot; bangunkan yang menunggu