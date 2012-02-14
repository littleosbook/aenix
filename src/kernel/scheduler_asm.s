[bits 32]

%include "constants.inc"

global enter_user_mode

USER_MODE_CS equ (SEGSEL_USER_SPACE_CS | 0x03) ; RPL = 0x3 to switch to PL3
USER_MODE_DS equ (SEGSEL_USER_SPACE_DS | 0x03) ; RPL = 0x3 to switch to PL3
ENABLE_INTERRUPTS equ 1000000000b   ; bit 9 is the IF value of EFLAGS

section .text
align 4
enter_user_mode:
    cli                             ; disable external interrupts
    mov     eax, [esp+4]            ; load address of registers_t into eax

    ; restore all the registers except eax
    mov     ebx, [eax+4]
    mov     ecx, [eax+8]
    mov     edx, [eax+12]
    mov     ebp, [eax+16]
    mov     esi, [eax+20]
    mov     edi, [eax+24]

    ; push information for iret onto the stack
    push    USER_MODE_DS            ; push the SS onto the stack
    push    DWORD [eax+28]          ; push the ESP of the user stack
    push    DWORD [eax+32]          ; push EFLAGS
    push    USER_MODE_CS            ; push the segment selector
    push    DWORD [eax+36]          ; push EIP, the CPU will start to exec init
    mov     eax, [eax]              ; restore eax

    ; move index for the user mode data segment with RPL = 3 into data registers
    push    ecx
    mov     cx, USER_MODE_DS
    mov     ds, cx
    mov     gs, cx
    mov     es, cx
    mov     fs, cx
    pop     ecx

    iret                            ; iret into user mode
