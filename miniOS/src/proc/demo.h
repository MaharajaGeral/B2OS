#pragma once
#include "kernel/types.h"
// Fungsi demo yang dipanggil dari perintah shell
void demo_thread();         // 2 thread, tugas berbeda, perubahan status tercetak
void demo_sched();          // 2 thread CPU-bound, bergantian karena timer
void demo_race();           // kriteria 4: shared counter tanpa dan dengan mutex
void demo_sem();            // kriteria 4: semaphore membatasi thread yang masuk
void show_threads();        // perintah 'ps'
void trace_enable(bool on); // perintah 'trace on|off'
bool trace_enabled();