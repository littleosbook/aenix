OBJECTS = loader.o kmain.o fb.o io.o gdt.o gdt_asm.o pic.o idt.o idt_asm.o \
		  interrupt.o interrupt_asm.o keyboard.o pit.o stdio.o string.o \
		  paging.o paging_asm.o kmalloc.o module.o serial.o log.o \
		  aefs.o process.o page_frame_allocator.o mem.o math.o tss.o \
		  tss_asm.o syscall.o scheduler.o scheduler_asm.o vfs.o devfs.o \
		  vnode.o
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
		 -nostartfiles -nodefaultlibs -Wall -Wextra -Werror -fomit-frame-pointer \
		 -Wno-unused-function -c
LDFLAGS = -T link.ld -melf_i386
AS = nasm
ASFLAGS = -f elf
AS_HEADERS = constants.inc

all: kernel.elf

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	./c_to_nasm.sh $(AS_HEADERS)
	$(AS) $(ASFLAGS) $<

kernel.elf: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

clean:
	rm -rf *.o kernel.elf *.inc
