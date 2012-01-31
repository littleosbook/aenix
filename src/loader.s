; this functions as a bridge between the bootloader (GRUB) and the kernel
; the main purpose of this file it to set up symbols for the bootloader
; and jump to the kmain function in the kernel

global loader                           ; the entry point for the linker

extern kmain                            ; kmain is defined in kmain.c
extern kernel_virtual_end               ; these are defined in the link script
extern kernel_virtual_start
extern kernel_physical_end
extern kernel_physical_start

; setting up the multiboot headers for GRUB
MODULEALIGN equ 1<<0                    ; align loaded modules on page
                                        ; boundaries
MEMINFO     equ 1<<1                    ; provide memory map
FLAGS       equ MODULEALIGN | MEMINFO   ; the multiboot flag field
MAGIC       equ 0x1BADB002              ; magic number for bootloader to
                                        ; find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum required

; because mistyping is easy
FOUR_KB         equ 0x00001000
FOUR_MB         equ 0x00400000

; paging for the kernel
KERNEL_VIRTUAL_BASE equ 0xC0000000                  ; we start at 3GB
KERNEL_PDT_IDX      equ KERNEL_VIRTUAL_BASE >> 22   ; index = highest 10 bits

; we set Page write-through, Writable, Present
KERNEL_PT_CFG       equ 00000000000000000000000000001011b
KERNEL_PDT_ID_MAP   equ 00000000000000000000000010001011b

; stack management
; the stack grows from the end of the page towards lower address
KERNEL_STACK_SIZE equ 0x400

; the page directory used to boot the kernel into the higher half
section .data
align 4096
kernel_pt:
    times 1024 dd 0
kernel_pdt:
    dd KERNEL_PDT_ID_MAP
    times 1023 dd 0

section .data
align 4
grub_magic_number:
    dd 0
grub_multiboot_info:
    dd 0

section .bss
align 4
kernel_stack:
    resb KERNEL_STACK_SIZE

section .text
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; the entry point, called by GRUB
loader:
    mov ecx, (grub_magic_number - KERNEL_VIRTUAL_BASE)
    mov [ecx], eax
    mov ecx, (grub_multiboot_info - KERNEL_VIRTUAL_BASE)
    mov [ecx], ebx

set_up_kernel_pdt:
    ; set up kernel_pdt to point to kernel_pt
    mov ecx, (kernel_pdt - KERNEL_VIRTUAL_BASE + KERNEL_PDT_IDX*4)
    mov edx, (kernel_pt - KERNEL_VIRTUAL_BASE)
    or  edx, KERNEL_PT_CFG
    mov [ecx], edx

set_up_kernel_pt:
    mov eax, (kernel_pt - KERNEL_VIRTUAL_BASE)
    mov ecx, KERNEL_PT_CFG
.loop:
    mov [eax], ecx
    add eax, 4
    add ecx, FOUR_KB
    cmp ecx, kernel_physical_end
    jle .loop

enable_paging:
    mov ecx, (kernel_pdt - KERNEL_VIRTUAL_BASE)
    and ecx, 0xFFFFF000     ; we only care about the upper 20 bits
    or  ecx, 0x08           ; PWT, enable page write through?
    mov cr3, ecx            ; load pdt

    mov ecx, cr4            ; read current config from cr4
    or  ecx, 0x00000010     ; set bit enabling 4MB pages
    mov cr4, ecx            ; enable it by writing to cr4

    mov	ecx, cr0	        ; read current config from cr0
    or  ecx, 0x80000000	    ; the highest bit controls paging
    mov cr0, ecx	        ; enable paging by writing config to cr0

    lea ecx, [higher_half]  ; store the address higher_half in ecx
    jmp ecx                 ; now we jump into 0xC0100000

; code executing from here on uses the page table, and is accessed through
; the upper half, 0xC0100000
higher_half:
    mov [kernel_pdt], DWORD 0
    mov esp, kernel_stack+KERNEL_STACK_SIZE  ; set up the stack

enter_kmain:
    push kernel_pdt
    push kernel_virtual_end             ; these are used by kmain, see
    push kernel_virtual_start           ; kernel_limits_t in kmain.c
    push kernel_physical_end
    push kernel_physical_start
    push DWORD [grub_magic_number]
    push DWORD [grub_multiboot_info]

    call kmain                          ; call the main function of the kernel
hang:
    jmp hang                            ; loop forever

