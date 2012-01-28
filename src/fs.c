#include "fs.h"
#include "string.h"
#include "log.h"

static inode_t *root;

void fs_init(uint32_t root_addr)
{
    root = (inode_t *) root_addr;
}

static inode_t *find_inode_in_dir(inode_t *dir, char *path, size_t fname_len)
{
    uint32_t num_files, i;

    if (dir->type != FILETYPE_DIR) {
        return NULL;
    }

    num_files = dir->size / sizeof(direntry_t);
    direntry_t *entry = (direntry_t *) (dir+1);
    for (i = 0; i < num_files; ++i, ++entry) {
        if (strncmp(path, entry->name, fname_len) == 0) {
            return (inode_t *) ((uint32_t) root + entry->location);
        }
    }

    return NULL;
}

inode_t *fs_find_inode(char *path)
{
    size_t path_len = strlen(path);
    size_t slash_index = 0;
    inode_t *node = root;

    if (path_len == 0) {
        return NULL;
    }

    if (path[0] != '/') {
        return NULL;
    }

    while (strlen(path) != 0 && node != NULL) {
        ++path; /* move past the '/' character */
        slash_index = strcspn(path, "/");
        node = find_inode_in_dir(node, path, slash_index);
        path += slash_index;
    }

    return node;
}

uint32_t fs_get_addr(inode_t *node)
{
    return (uint32_t) root + node->location;
}
