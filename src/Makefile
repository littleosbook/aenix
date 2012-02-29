APPS = init sh
FS_ROOT = fs_root
BIN_PATH = $(FS_ROOT)/bin
WARNINGS = -Wall -Wextra -Werror
CFLAGS = -m32
CC = gcc
FS_SIZE = 4 # in MB

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
	./mkfs $(FS_ROOT) $(FS_SIZE) $@
	@echo ''

mkfs_release: CFLAGS = $(CFLAGS) -O2
mkfs_release: mkfs

mkfs_debug: CFLAGS = $(CFLAGS) -g
mkfs_debug: mkfs

mkfs: mkfs.c
	$(CC) $(WARNINGS) $(CFLAGS) mkfs.c -o mkfs

rdaefs_release: CFLAGS = $(CFLAGS) -O2
rdaefs_release: rdaefs

rdaefs_debug: CFLAGS = $(CFLAGS) -g
rdaefs_debug: rdaefs

rdaefs: rdaefs.c
	$(CC) $(WARNINGS) $(CFLAGS) rdaefs.c -o rdaefs

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
	@echo '-> Cleaning kernel'
	@make --no-print-directory -C kernel clean
	@echo ''
	@echo '-> Cleaning apps'
	@make --no-print-directory -C apps clean
	@echo ''
	@echo '-> Cleaning libc'
	@make --no-print-directory -C libc clean
	@echo ''
	rm -rf mkfs aenix.iso fs rdaefs
