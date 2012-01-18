; these are functions dealing with the gdt and idt
; see also descriptor_tables.[c,h]

global gdt_load_and_set

KERNEL_CODE_SEGMENT_OFFSET equ 0x08
KERNEL_DATA_SEGMENT_OFFSET equ 0x10

section .text

; load the gdt into the cpu, and enter the kernel segments
gdt_load_and_set:
    mov     eax, [esp+4]        ; fetch gdt_ptr from parameter stack
    lgdt    [eax]               ; load gdt table
 
    ; load cs segment by doing a far jump
    jmp     KERNEL_CODE_SEGMENT_OFFSET:.reload_segments

.reload_segments:
    ; we only use one segment for data
    mov     ax, KERNEL_DATA_SEGMENT_OFFSET
    mov     ds, ax
    mov     ss, ax
    mov     es, ax
    mov     gs, ax
    mov     fs, ax
    ret
