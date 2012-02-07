APPS = init
FS_ROOT = fs_root
BIN_PATH = $(FS_ROOT)/bin

all: iso

iso: build_kernel build_libc build_apps fs
	./create_iso.sh fs

run: iso
	./run_bochs.sh || echo "\n" ; cat com1.out

create_bin_folder:
	mkdir -p $(BIN_PATH)
	for app in $(APPS); do cp apps/$$app $(BIN_PATH); done

fs: mkfs create_bin_folder
	./mkfs $(FS_ROOT) $@

mkfs: mkfs.c
	gcc -Wall -Wextra -Werror -O2 mkfs.c -o mkfs

build_apps:
	for app in $(APPS); do make -C apps $$app; done

build_kernel:
	make -C kernel

build_libc:
	make -C libc

clean:
	make -C kernel clean
	make -C apps clean
	make -C libc clean
	rm -rf mkfs aenix.iso fs
