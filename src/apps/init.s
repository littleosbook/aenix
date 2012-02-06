[bits 32]
section .text:
align 4
    mov edx, 0xBEEFCAFE
    int 0xAE
    mov edx, 0xCAFEBEEF
    int 0xAE
    mov edx, 0xDEADBEEF
loop:
    jmp loop
