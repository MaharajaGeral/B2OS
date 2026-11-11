/* void gdt_flush(u32 gdt_ptr) : muat GDT lalu reload segment register */
.intel_syntax noprefix
.section .text
.global gdt_flush
gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    push 0x08
    push offset 1f
    lret
1:  ret

.section .note.GNU-stack,"",@progbits