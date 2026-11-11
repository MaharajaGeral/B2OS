/* boot.S - header Multiboot + entry point (kriteria 1) */
.intel_syntax noprefix

.set MB_MAGIC,    0x1BADB002
.set MB_FLAGS,    0
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

.section .multiboot, "a"
.align 4
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECKSUM

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:

.section .text
.global _start
_start:
    cli
    mov esp, offset stack_top
    push ebx
    push eax
    call kernel_main
1:  cli
    hlt
    jmp 1b

.section .note.GNU-stack,"",@progbits