#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "inode.h"

#define DIRENT_FNAME_SIZE 256
#define SLASH_LEN 1
#define READ_BUFFER_LEN 4096

FILE *fs;

uint32_t visit_dir(char *path);
uint32_t visit_file(char *path);

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Bad root dir specified\n");
        fprintf(stderr, "Usage: %s root_dir output_file\n", argv[0]);
        exit(1);
    }
    if ((fs = fopen(argv[2], "w")) == NULL) {
        perror("ERROR: Couldn't open fs to write");
        exit(1);
    }
    if (fs == NULL) {
        perror("ERROR: couldn't open output file");
        exit(1);
    }
    visit_dir(argv[1]);
    fclose(fs);

    return 0;
}

uint32_t visit_dir(char *path)
{
    DIR *dir;
    struct dirent *ent;
    long dirpos;
    uint32_t num_files;
    char *child_path;
    uint32_t child_addr;
    struct stat st;
    direntry_t *entries;
    uint32_t i;
    inode_t dir_inode;

    child_path =
        malloc(sizeof(char)*(strlen(path) + DIRENT_FNAME_SIZE + SLASH_LEN));
    if (child_path == NULL) {
        perror("ERROR: out of memory!");
        exit(1);
    }

    dir = opendir(path);
    if (dir == NULL) {
        perror("ERROR: couldn't open directory");
        exit(1);
    }

    while ((ent = readdir(dir)) != NULL) {
        num_files++;
    }
    entries = (direntry_t *) malloc(sizeof(direntry_t) * num_files);
    if (entries == NULL) {
        perror("ERROR: out of memory!");
        exit(1);
    }

    rewinddir(dir);

    if ((dirpos = ftell(fs)) == -1) {
        perror("ERROR: Couldn't ftell file");
        exit(1);
    }

    if (fseek(fs, sizeof(inode_t) + sizeof(direntry_t) * num_files, SEEK_CUR)
        == -1) {
        perror("ERROR: Couldn't fseek");
        exit(1);
    }

    i = 0;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        if (sprintf(child_path, "%s/%s", path, ent->d_name) == -1) {
            perror("ERROR: Couldn't sprintf");
            exit(1);
        }
        if (stat(child_path, &st)) {
            perror("ERROR: Couldn't stat file");
            exit(1);
        }
        if (S_ISREG(st.st_mode)) {
            child_addr = visit_file(child_path);
        } else if (S_ISDIR(st.st_mode)) {
            child_addr = visit_dir(child_path);
        } else {
            fprintf(stderr, "ERROR: Unsupported file type: %s", child_path);
            exit(1);
        }
        entries[i].location = child_addr;
        strncpy(entries[i].name, ent->d_name, FILENAME_MAX_LEN);
        ++i;
    }

    dir_inode.type = FILETYPE_DIR;
    dir_inode.location = (uint32_t)(dirpos + sizeof(inode_t));
    dir_inode.size = sizeof(direntry_t) * num_files;

    if (fseek(fs, dirpos, SEEK_SET) == -1) {
        perror("ERROR: Couldn't fseek");
        exit(1);
    }

    if (fwrite(&dir_inode, sizeof(inode_t), 1, fs) != 1) {
        perror("ERROR: Couldn't write");
        exit(1);
    }
    if (fwrite(entries, sizeof(direntry_t), num_files, fs) != num_files) {
        perror("ERROR: Couldn't write");
        exit(1);
    }

    free(entries);
    free(child_path);
    if (closedir(dir)) {
        perror("ERROR: Couldn't close dir");
        exit(1);
    }

    return (uint32_t) dirpos;
}

uint32_t visit_file(char *path)
{
    inode_t file_inode;
    FILE *file;
    char buffer[READ_BUFFER_LEN];
    struct stat st;
    long pos;
    size_t bytes_read;

    if (stat(path, &st)) {
        perror("ERROR: Couldn't stat file");
        exit(1);
    }

    if ((pos = ftell(fs)) == -1) {
        perror("ERROR: Couldn't ftell");
        exit(1);
    }

    file_inode.type = FILETYPE_REG;
    file_inode.size = st.st_size;
    file_inode.location = (uint32_t)(pos + sizeof(inode_t));

    if (fwrite(&file_inode, sizeof(inode_t), 1, fs) != 1) {
        perror("ERROR: Couldn't write");
        exit(1);
    }

    if ((file = fopen(path, "r")) == NULL) {
        perror("ERROR: Couldn't open file for read");
        exit(1);
    }

    while ((bytes_read = fread(buffer, sizeof(char), READ_BUFFER_LEN, file))) {
        if (fwrite(buffer, sizeof(char), bytes_read, fs) != bytes_read) {
            perror("ERROR: Couldn't write");
            exit(1);
        }
    }

    fclose(file);
    return (uint32_t) pos;
}
