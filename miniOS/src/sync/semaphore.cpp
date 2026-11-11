// Semaphore (kriteria 4)
#include "sync/semaphore.h"
#include "proc/scheduler.h"
#include "kernel/cpu.h"

void sem_init(Semaphore* s, int initial) {
    s->count = initial;
    s->head  = 0;
    s->size  = 0;
    for (int i = 0; i < thread::MAX; i++) s->waiters[i] = nullptr;
}

void sem_wait(Semaphore* s) {
    u32 f = irq_save();
    if (s->count > 0) {
        s->count--;                       // masih ada slot
    } else {
        s->waiters[(s->head + s->size) % thread::MAX] = sched::current();
        s->size++;
        sched::block();                   // slot diberikan langsung oleh sem_signal()
    }
    irq_restore(f);
}

void sem_signal(Semaphore* s) {
    u32 f = irq_save();
    if (s->size > 0) {
        Thread* next = s->waiters[s->head];
        s->waiters[s->head] = nullptr;
        s->head = (s->head + 1) % thread::MAX;
        s->size--;
        sched::unblock(next);             // slot langsung dipakai thread ini
    } else {
        s->count++;
    }
    irq_restore(f);
}