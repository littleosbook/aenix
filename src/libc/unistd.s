[bits 32]

global syscall

section .text
align 4
; syscall
; - Performs a syscall, the stack has to be set up in advance. If the C
;   syscall declaration is used in unistd.h, the stack will be correct
;   due to the cdecl calling convention
syscall:
    add esp, 4	; do not send the return address to the kernel
    int 0xAE	; trap into the kernel
    sub esp, 4	; restore the return address (given that kernel didn't fuck up)
    ret
