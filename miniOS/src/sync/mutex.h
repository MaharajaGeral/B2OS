#pragma once
#include "proc/thread.h"

// Mutex: hanya satu thread boleh memegang kunci. Thread lain yang meminta akan
// berstatus WAITING dalam antrean FIFO sampai kunci dilepas.
struct Mutex {
    bool        locked;
    Thread*     owner;
    const char* name;
    Thread*     waiters[thread::MAX];   // antrean thread yang menunggu
    int         head;
    int         count;
};

void mutex_init(Mutex* m, const char* name);
void mutex_lock(Mutex* m);
void mutex_unlock(Mutex* m);