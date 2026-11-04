#pragma once
#include "kernel/types.h"
namespace pic {
void init();        // pindahkan IRQ ke vektor 32-47, aktifkan hanya IRQ0 (timer)
void eoi(int irq);  // beri tahu PIC bahwa interrupt sudah selesai ditangani
}