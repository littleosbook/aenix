OBJECTS = loader.o kmain.o fb.o io.o gdt.o gdt_asm.o pic.o idt.o idt_asm.o \
		  interrupt.o interrupt_asm.o keyboard.o pit.o stdio.o string.o \
		  paging.o paging_asm.o kmalloc.o module.o serial.o log.o \
		  enter_user_mode.o fs.o process.o page_frame_allocator.o mem.o math.o
CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
		 -nostartfiles -nodefaultlibs -Wall -Wextra -Werror \
		 -Wno-unused-function -c
LDFLAGS = -T link.ld -melf_i386
AS = nasm
ASFLAGS = -f elf
AS_HEADERS = constants.inc
MODULE_AS_FLAGS=-f bin
MKFS=mkfs.c
FS_ROOT=fs_root
BIN_PATH=$(FS_ROOT)/bin

all: kernel.elf

run: iso
	./run_bochs.sh || echo "\n" ; cat com1.out

iso: kernel.elf fs
	./create_iso.sh fs

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	./c_to_nasm.sh $(AS_HEADERS)
	$(AS) $(ASFLAGS) $<

kernel.elf: $(OBJECTS)
	ld $(LDFLAGS) $(OBJECTS) -o kernel.elf

fs_root:
	mkdir -p $(BIN_PATH)

mkfs: $(MKFS)
	$(CC) -Wall -Wextra -Werror -O2 $< -o $@

fs: mkfs fs_root init
	cp init $(BIN_PATH)/
	./mkfs $(FS_ROOT) $@

init: init.s
	$(AS) -f bin $< -o init

doc:
	cd .. ; doxygen doc/Doxyfile

clean:
	rm -rf *.o kernel.elf aenix.iso mkfs init fs *.inc
