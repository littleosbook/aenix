#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "kernel/aefs.h"

static void die(char const *msg)
{
    perror(msg);
    exit(1);
}

uint32_t div_ceil(uint32_t num, uint32_t den)
{
    return (num - 1) / den + 1;
}

struct block {
    char data[AEFS_BLOCK_SIZE];
} __attribute__((packed));
typedef struct block block_t;

static block_t *file;
static uint16_t num_inodes;
static block_t *block_start;

void read_superblock()
{
    aefs_superblock_t *sb = (aefs_superblock_t *) file;
    printf("-> Superblock\n");
    printf("\tmagic_number: %X\n", sb->magic_number);
    printf("\tnum_inodes: %hu\n", sb->num_inodes);
    printf("\tstart_block: %hu\n", sb->start_block);
    printf("\n");
    num_inodes = sb->num_inodes;
    block_start = file + sb->start_block;
}

aefs_inode_t *get_inode(uint16_t inode_id)
{
    return ((aefs_inode_t *)(file + 1)) + inode_id;
}

void print_inode_tail(aefs_inode_t *inode)
{
    uint32_t i = 0;
    for (; i < AEFS_INODE_NUM_BLOCKS; ++i) {
        printf("\tblock %u: %hu\n", i, inode->blocks[i]);
    }
    printf("\ttail: %hu\n", inode->inode_tail);
    if (inode->inode_tail != 0) {
        print_inode_tail(get_inode(inode->inode_tail));
    }
}

void print_inode(aefs_inode_t *inode) {
    printf("\ttype: %hhu\n", inode->type);
    printf("\tsize_high: %hhu\n", inode->size_high);
    printf("\tsize_low: %hu\n", inode->size_low);
    uint32_t i = 0;
    for (; i < AEFS_INODE_NUM_BLOCKS; ++i) {
        printf("\tblock %u: %hu\n", i, inode->blocks[i]);
    }
    printf("\ttail: %hu\n", inode->inode_tail);
    if (inode->inode_tail != 0) {
        print_inode_tail(get_inode(inode->inode_tail));
    }
}

void visit_reg(aefs_inode_t *inode, char const *path);
void visit_dir(aefs_inode_t *inode, char const *path);

void visit_inode(aefs_inode_t *inode, char const *path)
{
    if (AEFS_INODE_IS_REG(inode)) {
        visit_reg(inode, path);
    } else if (AEFS_INODE_IS_DIR(inode)) {
        visit_dir(inode, path);
    } else {
        die("ERROR: Unknown file type");
    }
}

void print_root()
{
    aefs_inode_t *root_inode = get_inode(1);
    visit_inode(root_inode, "/");
}


void visit_reg(aefs_inode_t *inode, char const *path)
{
    printf("-> REG: %s\n", path);
    print_inode(inode);
}

void visit_dir(aefs_inode_t *inode, char const *path)
{
    int num_entries = AEFS_INODE_SIZE(inode)/sizeof(aefs_direntry_t);
    int i;
    aefs_direntry_t *entry;
    block_t *block;
    int block_num = 0;
    aefs_inode_t *inode_to_read_from = inode;
    printf("-> DIR: %s (%hu entries)\n", path, num_entries);
    print_inode(inode);

    char *child_path = malloc(strlen(path) + AEFS_FILENAME_MAX_LEN + 1);
    if (child_path == NULL) {
        die("ERROR: Out of memory");
    }

    for (i = 0; i < num_entries; ++i) {
        if (i % (AEFS_DIRENTRIES_PER_BLOCK) == 0) {
            block = block_start + inode_to_read_from->blocks[block_num];
            block_num++;
            if (block_num == AEFS_INODE_NUM_BLOCKS) {
                block_num = 0;
                inode_to_read_from = get_inode(inode_to_read_from->inode_tail);
            }
        }
        entry = ((aefs_direntry_t *) (block)) + (i % AEFS_DIRENTRIES_PER_BLOCK);
        printf("\t\t-> ENTRY: %s -> %hu\n", entry->name, entry->inode_id);

        sprintf(child_path, "%s/%s", path, entry->name);
        visit_inode(get_inode(entry->inode_id), child_path);
    }

    free(child_path);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Too many arguments\n");
        fprintf(stderr, "Usage: %s fs\n", argv[0]);
        exit(1);
    }


    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        die("ERROR: Can't open fs");
    }
    struct stat st;

    fstat(fd, &st);

    void *mem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mem == MAP_FAILED) {
        die("ERROR: Couldn't mmap file");
    }

    file = (block_t *) mem;

    read_superblock();
    print_root();

    if (munmap(mem, st.st_size)) {
        die("ERROR: Could non munmap file");
    }
    close(fd);

    return 0;
}
