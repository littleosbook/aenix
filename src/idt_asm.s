global idt_load_and_set

section .text

idt_load_and_set:
    mov eax, [esp+4]    ; move ldt_ptr to eax
    lidt [eax]          ; load the idt table stored in eax
    ret
