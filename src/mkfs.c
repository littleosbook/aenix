#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "kernel/inode.h"

#define DIRENT_FNAME_SIZE 256
#define SLASH_LEN 1
#define READ_BUFFER_LEN 4096

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
    char data[BLOCK_SIZE];
} __attribute__((packed));
typedef struct block block_t;

static block_t *file;
static uint16_t num_inodes;
static uint16_t current_inode_id = 1;
static uint16_t current_block_id = 0;
static inode_t *start_inode;
static block_t *start_block;

static void write_superblock();
static uint16_t visit_dir(char *path, int is_root);
static uint16_t visit_file(char *path);

void fill_inode_blocks(inode_t *inode, uint16_t blocks_required,
                       uint16_t start_block_id)
{
    uint32_t i;
    inode_t *list = inode;
    for (i = 0; i < blocks_required; ++i) {
        if (i % INODE_NUM_BLOCKS == 0 && i != 0) {
            list->inode_tail = current_inode_id;
            list = start_inode + current_inode_id;
            memset(list, 0, sizeof(inode_t));
            current_inode_id++;
        }
        list->blocks[i % INODE_NUM_BLOCKS] = start_block_id + i;
    }
}

void fill_inode_size(inode_t *inode, uint32_t size)
{
    inode->size_low = size & 0xFFFF;
    inode->size_high = (size >> 16) & 0xFF;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Bad root dir specified\n");
        fprintf(stderr, "Usage: %s root_dir fs_size (in MB) output_file\n",
                argv[0]);
        exit(1);
    }

    int fd;
    if ((fd = open(argv[3], O_RDWR | O_CREAT | O_TRUNC)) == -1) {
        die("ERROR: Couldn't open output file");
    }

    size_t size = (size_t) (atoi(argv[2]) << 20);
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (mem == MAP_FAILED) {
        die("ERROR: Couldn't mmap file");
    }
    file = (block_t *) mem;

    uint16_t blocks = size / BLOCK_SIZE;

    uint16_t inode_blocks = div_ceil(blocks*sizeof(inode_t), BLOCK_SIZE);
    num_inodes = blocks - inode_blocks - 1;
    start_block = file + 1 + inode_blocks;
    start_inode = (inode_t*)(file + 1) + 1; /* inode 0 is reserved */

    write_superblock();
    visit_dir(argv[1], 1);

    if (munmap(mem, size)) {
        die("ERROR: Couldn't munmap file");
    }
    close(fd);

    return 0;
}

static void write_superblock()
{
    superblock_t *sb = (superblock_t*) file;
    memset(sb, 0, BLOCK_SIZE);
    sb->num_inodes = num_inodes;
}

static uint16_t visit_dir(char *path, int is_root)
{
    DIR *dir;
    uint32_t i;
    struct dirent *ent;
    uint32_t num_files = 0;
    char *child_path;
    uint16_t child_inode_id;
    struct stat st;
    uint16_t dir_inode_id;
    inode_t *dir_inode;

    child_path =
        malloc(sizeof(char)*(strlen(path) + DIRENT_FNAME_SIZE + SLASH_LEN));
    if (child_path == NULL) {
        die("ERROR: out of memory!");
    }

    dir = opendir(path);
    if (dir == NULL) {
        die("ERROR: couldn't open directory");
    }

    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        num_files++;
    }

    if (is_root) {
        num_files++; /* to make room for /dev */
    }

    dir_inode_id = current_inode_id;
    dir_inode = start_inode + dir_inode_id;
    current_inode_id++;
    if (dir_inode_id == num_inodes) {
        die("ERROR: Too many inodes required");
    }

    uint32_t blocks_required = div_ceil(num_files*sizeof(direntry_t),
                                        BLOCK_SIZE);
    uint16_t dir_start_block_id = current_block_id;
    block_t *dir_start_block = start_block + current_block_id;
    current_block_id += blocks_required;
    memset(dir_start_block, 0, BLOCK_SIZE * blocks_required);
    direntry_t *entries = (direntry_t *) dir_start_block;

    rewinddir(dir);

    i = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        if (sprintf(child_path, "%s/%s", path, ent->d_name) == -1) {
            die("ERROR: Couldn't sprintf");
        }
        if (stat(child_path, &st)) {
            die("ERROR: Couldn't stat file");
        }
        if (S_ISREG(st.st_mode)) {
            child_inode_id = visit_file(child_path);
        } else if (S_ISDIR(st.st_mode)) {
            child_inode_id = visit_dir(child_path, 0);
        } else {
            fprintf(stderr, "ERROR: Unsupported file type: %s", child_path);
            exit(1);
        }
        entries[i].inode = child_inode_id;
        strncpy(entries[i].name, ent->d_name, FILENAME_MAX_LEN);
        ++i;
    }

    dir_inode->type = FILETYPE_DIR;
    uint32_t dir_size = sizeof(direntry_t) * num_files;
    fill_inode_size(dir_inode, dir_size);

    fill_inode_blocks(dir_inode, blocks_required, dir_start_block_id);

    free(child_path);
    if (closedir(dir)) {
        die("ERROR: Couldn't close dir");
    }

    return dir_inode_id;
}

uint16_t visit_file(char *path)
{
    FILE *file;
    struct stat st;
    size_t bytes_read;

    if (stat(path, &st)) {
        die("ERROR: Couldn't stat file");
    }

    uint16_t file_inode_id = current_inode_id;
    current_inode_id++;
    inode_t *file_inode = start_inode + file_inode_id;
    uint16_t blocks_required = div_ceil(st.st_size, BLOCK_SIZE);
    uint16_t file_start_block_id = current_block_id;
    block_t *file_start_block = start_block + file_start_block_id;
    current_block_id += blocks_required;

    file_inode->type = FILETYPE_REG;
    fill_inode_size(file_inode, st.st_size);

    if ((file = fopen(path, "r")) == NULL) {
        die("ERROR: Couldn't open file for read");
    }

    while ((bytes_read =
                fread(file_start_block++, sizeof(char), BLOCK_SIZE, file))) {
    }

    fill_inode_blocks(file_inode, blocks_required, file_start_block_id);

    fclose(file);
    return file_inode_id;
}
