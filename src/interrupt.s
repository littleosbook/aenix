extern pic_acknowledge      ; defined in pic.c

global enable_interrupts
global disable_interrupts
global handle_cpu_int
global handle_pic_int

section .text:

enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

handle_cpu_int:
    mov word [0xB8000], 0x0F43
    iret

handle_pic_int:
    mov     word [0xB8000], 0x0F44
    push    eax
    call    pic_acknowledge 
    pop     eax
    iret
