#pragma once
#include "kernel/types.h"
#include "proc/thread.h"

// Pop-up monitoring sementara: kotak kecil yang memperlihatkan aktivitas kernel,
// lalu menghilang beberapa detik setelah aktivitas terakhir.
// Semua fungsi event aman dipanggil dari interrupt: tidak memakai kmalloc, mutex,
// atau cetak layar. Penggambaran dikerjakan thread latar "monitor".
namespace monitor {
void init();
void thread_state(const Thread* t, ThreadState s);
void context_switch(const Thread* from, const Thread* to);
}
