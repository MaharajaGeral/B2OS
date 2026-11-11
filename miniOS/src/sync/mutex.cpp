// Mutex (kriteria 4, 5)
#include "sync/mutex.h"
#include "proc/scheduler.h"
#include "kernel/cpu.h"

void mutex_init(Mutex* m, const char* name) {
    m->locked = false;
    m->owner  = nullptr;
    m->name   = name;
    m->head   = 0;
    m->count  = 0;
    for (int i = 0; i < thread::MAX; i++) m->waiters[i] = nullptr;
}

void mutex_lock(Mutex* m) {
    u32 f = irq_save();
    Thread* me = sched::current();
    if (!m->locked) {
        m->locked = true;                 // kunci bebas -> langsung diambil
        m->owner  = me;
    } else {
        // Kunci dipegang thread lain: masuk antrean FIFO lalu tidur (WAITING).
        m->waiters[(m->head + m->count) % thread::MAX] = me;
        m->count++;
        sched::block();
        // Sampai di sini = mutex_unlock() sudah menyerahkan kepemilikan ke kita.
    }
    irq_restore(f);
}

void mutex_unlock(Mutex* m) {
    u32 f = irq_save();
    if (m->locked && m->owner == sched::current()) {
        if (m->count > 0) {
            // Serahkan langsung ke thread terdepan di antrean (locked tetap true).
            Thread* next = m->waiters[m->head];
            m->waiters[m->head] = nullptr;
            m->head = (m->head + 1) % thread::MAX;
            m->count--;
            m->owner = next;
            sched::unblock(next);
        } else {
            m->locked = false;
            m->owner  = nullptr;
        }
    }
    irq_restore(f);
}