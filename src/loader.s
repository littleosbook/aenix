; this functions as a bridge between the bootloader (GRUB) and the kernel
; the main purpose of this file it to set up symbols for the bootloader
; and jump to the kmain function in the kernel

; based on http://wiki.osdev.org/Bare_bones#NASM

global loader                           ; the entry point for the linker
global boot_page_directory

extern kmain                            ; kmain is defined in kmain.c
extern move_multiboot_modules           ; defined in module.c
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

; some paging constants
LARGE_PAGE_SIZE equ FOUR_MB
SMALL_PAGE_SIZE equ FOUR_KB

; paging for the kernel
KERNEL_VIRTUAL_BASE equ 0xC0000000                  ; we start at 3GB
KERNEL_PAGE_SIZE    equ LARGE_PAGE_SIZE             ; the page is 4 MB
KERNEL_PDT_IDX      equ KERNEL_VIRTUAL_BASE >> 22   ; index = highest 10 bits

; paging for the modules
MODULE_VIRTUAL_BASE equ (KERNEL_VIRTUAL_BASE + KERNEL_PAGE_SIZE)
MODULE_PAGE_SIZE    equ LARGE_PAGE_SIZE
MODULE_PDT_IDX      equ MODULE_VIRTUAL_BASE >> 22

; stack management
; the stack grows from the end of the page towards lower address
; the stack must be aligned at 4 bytes, hence -4 instead of -1
KERNEL_STACK_VIRTURAL_ADDRESS equ KERNEL_VIRTUAL_BASE + KERNEL_PAGE_SIZE - 4

; the page directory used to boot the kernel into the higher half
section .data
align 4096                               ; align on 4kB blocks
boot_page_directory:
    ; the following macro identity maps all the memory except 0xC0000000 that
    ; maps to 0x00000000
    %assign mem 0
    %rep    1024
        %if mem == KERNEL_VIRTUAL_BASE
            dd 00000000000000000000000010001011b
        %elif mem == MODULE_VIRTUAL_BASE
            dd (FOUR_MB | 00000000000000000000000010001011b)
        %else
            dd (mem | 00000000000000000000000010001011b)
        %endif
        %assign mem mem+FOUR_MB
    %endrep

section .text
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; the entry point, called by GRUB
loader:
set_up_paging:
    mov ecx, (boot_page_directory-KERNEL_VIRTUAL_BASE)
    and ecx, 0xFFFFF000     ; we only care about the upper 20 bits
    or  ecx, 0x08           ; PWT, enable page write through?
    mov cr3, ecx            ; load pdt

    mov ecx, cr4            ; read current config from cr4
    or  ecx, 0x00000010     ; set bit enabling 4MB pages
    mov cr4, ecx            ; enable it by writing to cr4

    mov	ecx, cr0	    ; read current config from cr0
    or  ecx, 0x80000000	    ; the highest bit controls paging
    mov cr0, ecx	    ; enable paging by writing config to cr0

    lea ecx, [higher_half]  ; store the address higher_half in ecx
    jmp ecx                 ; now we jump into 0xC0100000

; code executing from here on uses the page table, and is accessed through
; the upper half, 0xC0100000
higher_half:

call_move_multiboot_modules:
    mov     esp, mini_stack + MINI_STACK_SIZE   ; set up a temp min stack
    push    eax                                 ; save eax on the stack
    push    ebx                                 ; ebx = multiboot data
    call    move_multiboot_modules
    pop     ebx                                 ; restore ebx
    pop     eax                                 ; restora eax

restore_pdt:
    %assign i 0
    %assign mem 0
    %rep    1024
        %if i != KERNEL_PDT_IDX && i != MODULE_PDT_IDX
            mov DWORD [boot_page_directory + i*4], 0
            invlpg [mem]
        %endif
        %assign i i+1
        %assign mem mem+0x00400000
    %endrep

enter_kmain:
    mov esp, KERNEL_STACK_VIRTURAL_ADDRESS  ; set up the stack
    push boot_page_directory
    push kernel_virtual_end             ; these are used by kmain, see
    push kernel_virtual_start           ; kernel_limits_t in kmain.c
    push kernel_physical_end
    push kernel_physical_start
    push eax                            ; eax contains the MAGIC number
    push ebx                            ; ebx contains the multiboot data
                                        ; structure
    call kmain                          ; call the main function of the kernel
hang:
    jmp hang                            ; loop forever

MINI_STACK_SIZE equ 0x400
section .bss
align 4
mini_stack:
    resb MINI_STACK_SIZE
