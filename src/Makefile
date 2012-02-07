APPS = init
FS_ROOT = fs_root
BIN_PATH = $(FS_ROOT)/bin

all: iso

iso: build_kernel build_libc fs
	@echo '-> Creating ISO'
	./create_iso.sh fs
	@echo ''

run: iso
	@echo '-> Starting BOCHS'
	./run_bochs.sh || echo "\n" ; cat com1.out

create_bin_folder: build_apps
	@echo '-> Copying applications to fs folder'
	mkdir -p $(BIN_PATH)
	for app in $(APPS); do cp apps/$$app $(BIN_PATH); done
	@echo ''

fs: mkfs create_bin_folder
	@echo '-> Creating filesystem'
	./mkfs $(FS_ROOT) $@
	@echo ''

mkfs: mkfs.c
	gcc -Wall -Wextra -Werror -O2 mkfs.c -o mkfs

build_apps:
	@echo '-> Building applications'
	@make --no-print-directory -C apps
	@echo ''

build_kernel:
	@echo '-> Building kernel'
	@make --no-print-directory -C kernel
	@echo ''

build_libc:
	@echo '-> Building libc'
	@make --no-print-directory -C libc
	@echo ''

clean:
	make -C kernel clean
	make -C apps clean
	make -C libc clean
	rm -rf mkfs aenix.iso fs
