section .text:
align 4
    pop ebx
    int 0x86
    jmp ebx
