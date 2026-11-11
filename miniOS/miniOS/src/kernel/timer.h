#pragma once
#include "kernel/types.h"
namespace timer {
const u32 HZ = 100;     // 100 tick per detik (1 tick = 10 ms)
void init();
u32  ticks();
}