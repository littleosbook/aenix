[bits 32]
section .text:
align 4
    mov ecx, 0xCAFEBEEF
loop:
    jmp loop
