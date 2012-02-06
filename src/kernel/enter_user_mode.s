[bits 32]

global enter_user_mode

SEGSEL_USER_SPACE_CS equ (0x18 | 0x03) ; RPL = 0x3 to switch to PL3
SEGSEL_USER_SPACE_DS equ (0x20 | 0x03) ; RPL = 0x3 to switch to PL3
ENABLE_INTERRUPTS    equ 1000000000b   ; bit 9 is the IF value of EFLAGS

extern fb_put_ui_hex

section .text
align 4
enter_user_mode:
    cli                             ; disable external interrupts
    mov     eax, [esp+4]            ; load address of init into eax
    mov     ebx, [esp+8]            ; load address of stack into ebx
    push    SEGSEL_USER_SPACE_DS    ; push the SS onto the stack
    push    ebx                     ; push the ESP of the user stack

    pushf                           ; push the current value of EFLAGS
    pop     edx                     ; store the value of EFLAGS in edx
    or      edx, ENABLE_INTERRUPTS  ; set the IF bit to 1
    push    edx                     ; push the value of EFLAGS back on the stack

    push    SEGSEL_USER_SPACE_CS    ; push the segment selector
    push    eax                     ; push EIP, the CPU will start to exec init

    ; move index for the user mode data segment with RPL = 3 into data registers
    mov     cx, SEGSEL_USER_SPACE_DS
    mov     ds, cx
    mov     gs, cx
    mov     es, cx
    mov     fs, cx

    iret                            ; iret into user mode
