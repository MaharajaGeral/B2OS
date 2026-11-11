/* Simpan/pulihkan register antar thread (kriteria 3) */
/* void switch_context(u32* old_esp, u32 new_esp)
 * Simpan register thread lama + ESP-nya, lalu pindah ke stack thread baru.
 * Inilah context switch yang sebenarnya (kriteria 3). */
.intel_syntax noprefix
.section .text
.global switch_context
switch_context:
    mov eax, [esp + 4]      /* tempat menyimpan esp thread lama */
    mov edx, [esp + 8]      /* esp thread baru */
    push ebp
    push ebx
    push esi
    push edi
    mov [eax], esp          /* simpan stack pointer thread lama */
    mov esp, edx            /* pindah ke stack thread baru */
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret                     /* lanjut ke tempat thread baru terakhir berhenti */

.section .note.GNU-stack,"",@progbits