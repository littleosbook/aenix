[bits 32]

extern main
extern syscall

SYS_exit equ 6

section .text
align 4
    call main
    push eax        ; eax is the status code from main, send it to exit
    push SYS_exit
    call syscall
    jmp  $          ; we'll never get here, but if, loop indefinitely
