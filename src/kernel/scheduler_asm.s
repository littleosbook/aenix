[bits 32]

%include "constants.inc"

global run_process_in_user_mode
global run_process_in_kernel_mode
global snapshot_and_schedule

extern fb_put_b
extern fb_put_ui_hex
extern scheduler_schedule

section .text
align 4
run_process_in_user_mode:
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
    push    DWORD [eax+28]          ; push the SS onto the stack
    push    DWORD [eax+32]          ; push the ESP of the user stack
    push    DWORD [eax+36]          ; push EFLAGS
    push    DWORD [eax+40]          ; push the segment selector
    push    DWORD [eax+44]          ; push EIP

    ; move index for the data segment into data registers
    push    ecx
    mov     cx, [eax+28]
    mov     ds, cx
    mov     gs, cx
    mov     es, cx
    mov     fs, cx
    pop     ecx

    mov     eax, [eax]              ; restore eax

    iret                            ; iret into the given mode

run_process_in_kernel_mode:
    cli                             ; disable external interrupts
    mov     eax, [esp+4]            ; load address of registers_t into eax

    ; restore all the registers except eax
    mov     ebx, [eax+4]
    mov     ecx, [eax+8]
    mov     edx, [eax+12]
    mov     ebp, [eax+16]
    mov     esi, [eax+20]
    mov     edi, [eax+24]

    ; restore the stack pointer
    mov     esp, [eax+32]

    ; push information for iret onto the stack
    push    DWORD [eax+36]          ; push EFLAGS
    push    DWORD [eax+40]          ; push the segment selector
    push    DWORD [eax+44]          ; push EIP

    mov     eax, [eax]              ; restore eax

    iret                            ; iret to return to the process

snapshot_and_schedule:
    cli                             ; disable external interrupts
    mov     eax, [esp+4]            ; load address of registers_t into eax

    ; restore all the registers except eax, since eax holds return value
    mov     DWORD [eax], 0          ; 0 => the function call was succesfull
    mov     [eax+4],  ebx
    mov     [eax+8],  ecx
    mov     [eax+12], edx
    mov     [eax+16], ebp
    mov     [eax+20], esi
    mov     [eax+24], edi
    mov     [eax+28], ss
    mov     [eax+32], esp
    add     DWORD [eax+32], 4

    ; snapshot eflags
    pushf
    pop     DWORD [eax+36]

    mov     [eax+40], cs
    mov     ebx, [esp]
    mov     [eax+44], ebx           ; jump back to the calling code

    call    scheduler_schedule
    jmp     $
