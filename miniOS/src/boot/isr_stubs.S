.intel_syntax noprefix
/* Stub interrupt: tiap vektor punya pintu masuk sendiri, lalu semuanya menuju isr_common. */
.section .text
.extern isr_handler

.macro ISR_NOERR n
.global isr\n
isr\n:
    push 0              /* error code palsu supaya bentuk stack sama */
    push \n             /* nomor interrupt */
    jmp isr_common
.endm

.macro ISR_ERR n
.global isr\n
isr\n:
    push \n             /* CPU sudah menaruh error code */
    jmp isr_common
.endm

.irp n, 0,1,2,3,4,5,6,7,9,15,16,18,19,20,21,22,23,24,25,26,27,28,29,31
    ISR_NOERR \n
.endr
.irp n, 8,10,11,12,13,14,17,30
    ISR_ERR \n
.endr
.irp n, 32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
    ISR_NOERR \n
.endr

isr_common:
    pushad              /* simpan semua register */
    push esp            /* argumen: pointer ke Regs */
    call isr_handler
    add esp, 4
    popad               /* pulihkan register */
    add esp, 8          /* buang int_no dan err_code */
    iret

.section .data
.global isr_stub_table
isr_stub_table:
.irp n, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
    .long isr\n
.endr

.section .note.GNU-stack,"",@progbits