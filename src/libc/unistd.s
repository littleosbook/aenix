[bits 32]

global syscall

section .text
align 4
; syscall
; - Performs a syscall, the stack has to be set up in advance. If the C
;   syscall declaration is used in unistd.h, the stack will be correct
;   due to the cdecl calling convention
syscall:
    int 0xAE
    ret
