[bits 32]

global enter_user_mode

SEGSEL_USER_SPACE_CS equ (0x18 | 0x03) ; RPL = 0x3 to switch to PL3
SEGSEL_USER_SPACE_DS equ (0x20 | 0x03) ; RPL = 0x3 to switch to PL3

section .text
align 4
enter_user_mode:
    mov     eax, [esp+4]            ; load address of init into eax
    mov     ebx, [esp+8]            ; load address of stack into ebx
    push    SEGSEL_USER_SPACE_DS    ; push the SS onto the stack
    push    ebx                     ; push the ESP of the user stack
    pushf                           ; push the current value of EFLAGS
    push    SEGSEL_USER_SPACE_CS    ; push the segment selector
    push    eax                     ; push EIP, the CPU will start to exec init

    ; move index for the user mode data segment with RPL = 3 into data registers
    mov     ax, SEGSEL_USER_SPACE_DS
    mov     ds, ax
    mov     gs, ax
    mov     es, ax
    mov     fs, ax

    iret                            ; iret into user mode
