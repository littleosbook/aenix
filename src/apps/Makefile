CC = gcc
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
		 -nostartfiles -nodefaultlibs -Wall -Wextra -Werror \
		 -Wno-unused-function
AS = nasm
ASFLAGS = -f bin

init: init.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf init
