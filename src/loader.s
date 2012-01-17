; this functions as a bridge between the bootloader (GRUB) and the kernel
; the main purpose of this file it to set up symbols for the bootloader
; and jump to the kmain function in the kernel

; based on http://wiki.osdev.org/Bare_bones#NASM

global loader                           ; the entry point for the linker

extern kmain                            ; kmain is defined in kmain.c

; setting up the multiboot headers for GRUB
MODULEALIGN equ 1<<0                    ; align loaded modules on page 
                                        ; boundaries
MEMINFO     equ 1<<1                    ; provide memory map
FLAGS       equ MODULEALIGN | MEMINFO   ; the multiboot flag field
MAGIC       equ 0x1BADB002              ; magic number for bootloader to 
                                        ; find the header
CHECKSUM    equ -(MAGIC + FLAGS)        ; checksum required

section .text

align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; the entry point, called by GRUB
loader:
    mov esp, stack+STACKSIZE            ; sets up the stack pointer
    push eax                            ; eax contains the MAGIC number
    push ebx                            ; ebx contains the multiboot data 
                                        ; structure
    cli                                 ; disable interrupts
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
