; this functions as a bridge between the bootloader (GRUB) and the kernel
; the main purpose of this file it to set up symbols for the bootloader
; and jump to the kmain function in the kernel

; based on http://wiki.osdev.org/Bare_bones#NASM

global loader                           ; the entry point for the linker

extern kmain                            ; kmain is defined in kmain.c
extern end_of_kernel

; setting up the multiboot headers for GRUB
MODULEALIGN equ 1<<0                    ; align loaded modules on page 
                                        ; boundaries
MEMINFO     equ 1<<1                    ; provide memory map
FLAGS       equ MODULEALIGN | MEMINFO   ; the multiboot flag field
MAGIC       equ 0x1BADB002              ; magic number for bootloader to 
                                        ; find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum required

; paging for the kernel
KERNEL_VIRTUAL_BASE     equ 0xC0000000                  ; we start at 3GB
KERNEL_PAGE_IDX         equ (KERNEL_VIRTUAL_BASE >> 22) ; PDT index for 4MB PDE

; the page directory used to boot the kernel into the higher half
section .data
align 4096                               ; align on 4kB blocks
boot_page_directory:
    ; we want, from left to right:
    ; 4MB page, Page write-through, Writable, Present
    dd 00000000000000000000000010001011b ; identity mapped first 4MB
    times (KERNEL_PAGE_IDX-1) dd 0       ; no pages here
    dd 00000000000000000000000010001011b ; map 0xC0000000 to the first 4MB
    times (1024-KERNEL_PAGE_IDX-1) dd 0  ; no more pages


section .text
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; the entry point, called by GRUB
loader:
    mov ecx, (boot_page_directory-KERNEL_VIRTUAL_BASE)
    and ecx, 0xFFFFF000     ; we only care about the upper 20 bits
    or  ecx, 0x08           ; PWT, enable page write through?
    mov cr3, ecx            ; load pdt

    mov ecx, cr4            ; read current config from cr4
    or  ecx, 0x00000010     ; set bit enabling 4MB pages
    mov cr4, ecx            ; enable it by writing to cr4

    mov	ecx, cr0		    ; read current config from cr0
	or	ecx, 0x80000000	    ; the highest bit controls paging
	mov cr0, ecx		    ; enable paging by writing config to cr0

    lea ecx, [higher_half]  ; store the address higher_half in ecx
    jmp ecx                 ; now we jump into 0xC0100000

; code executing from here on uses the page table, and is accessed through
; the upper half, 0xC0100000
higher_half:
    mov     DWORD [boot_page_directory], 0  ; erase identity mapping of kernel
    invlpg  [0]                             ; and flush any tlb-references to it

    mov esp, stack+STACKSIZE            ; sets up the stack pointer
    push end_of_kernel
    push eax                            ; eax contains the MAGIC number
    push ebx                            ; ebx contains the multiboot data 
                                        ; structure
    call kmain                          ; call the main function of the kernel

hang:
    jmp hang                            ; loop forever

; reserve initial stack space
STACKSIZE equ 0x4000                    ; 16kB

section .bss
align 4
stack:
    resb STACKSIZE                      ; reserve memory for stack on 
                                        ; doubleworded memory
