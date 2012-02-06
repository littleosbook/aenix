global tss_load_and_set

section .text
tss_load_and_set:
    mov ax, [esp+4]     ; mov the tss segsel into ax (16 bits)
    ltr ax              ; load the task register with the selector
    ret
