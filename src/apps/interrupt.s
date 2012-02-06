[bits 32]

global interrupt

section .text
align 4
interrupt:
    INT 0xAE
    ret
