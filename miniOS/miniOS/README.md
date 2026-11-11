# NusaOS (Mini-OS)

Kernel x86 32-bit sederhana, C++ freestanding, berjalan di QEMU.

## Persiapan (Ubuntu / WSL2)
    sudo apt update
    sudo apt install build-essential qemu-system-x86 gdb git grub-pc-bin grub-common xorriso mtools gcc-multilib g++-multilib

## Build & jalankan
    make run        # build + jalankan di QEMU (cepat)
    make run-iso    # build ISO GRUB lalu jalankan
    make clean

Perintah shell: help, info, clear, echo, uptime, ps, trace, banner, demo_thread, demo_sched
