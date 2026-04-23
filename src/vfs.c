#include "../include/vfs.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/screen.h"

typedef struct {
    char  name[MAX_NAME_LEN];
    int   size;
    int   is_dir;
    int   is_used;
    char *data;        /* Pointer into heap (via memory.c) */
    int   parent;      /* Parent inode index (-1 for root) */
} Inode;

typedef struct {
    int  total_files;
    int  total_dirs;
    int  current_dir;  /* Current working directory inode index */
} Superblock;

static Inode      inode_table[MAX_INODES];
static Superblock superblock;

void vfs_init(void)
{
    superblock.total_files = 0;
    superblock.total_dirs  = 1;  /* root */
    superblock.current_dir = 0;

    /* Clear inode table */
    for (int i = 0; i < MAX_INODES; i++) {
        inode_table[i].is_used = 0;
        inode_table[i].data    = NULL;
        inode_table[i].size    = 0;
        inode_table[i].is_dir  = 0;
        inode_table[i].parent  = -1;
        inode_table[i].name[0] = '\0';
    }

    /* Create root directory (inode 0) */
    str_copy(inode_table[0].name, "/", MAX_NAME_LEN);
    inode_table[0].is_used = 1;
    inode_table[0].is_dir  = 1;
    inode_table[0].parent  = -1;
}

static int vfs_find_free_inode(void)
{
    for (int i = 0; i < MAX_INODES; i++) {
        if (!inode_table[i].is_used) return i;
    }
    return -1;
}

static int vfs_find_by_name(const char *name, int parent)
{
    for (int i = 0; i < MAX_INODES; i++) {
        if (inode_table[i].is_used &&
            inode_table[i].parent == parent &&
            str_compare(inode_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int vfs_touch(const char *name)
{
    if (!name || str_length(name) == 0) {
        scr_print("  Error: filename required\n");
        return -1;
    }

    /* Check if already exists */
    if (vfs_find_by_name(name, superblock.current_dir) >= 0) {
        scr_print("  Error: '");
        scr_print(name);
        scr_print("' already exists\n");
        return -1;
    }

    int idx = vfs_find_free_inode();
    if (idx < 0) {
        scr_print("  Error: inode table full\n");
        return -1;
    }

    str_copy(inode_table[idx].name, name, MAX_NAME_LEN);
    inode_table[idx].is_used = 1;
    inode_table[idx].is_dir  = 0;
    inode_table[idx].size    = 0;
    inode_table[idx].data    = NULL;
    inode_table[idx].parent  = superblock.current_dir;

    superblock.total_files++;
    return idx;
}

int vfs_mkdir(const char *name)
{
    if (!name || str_length(name) == 0) {
        scr_print("  Error: directory name required\n");
        return -1;
    }

    if (vfs_find_by_name(name, superblock.current_dir) >= 0) {
        scr_print("  Error: '");
        scr_print(name);
        scr_print("' already exists\n");
        return -1;
    }

    int idx = vfs_find_free_inode();
    if (idx < 0) {
        scr_print("  Error: inode table full\n");
        return -1;
    }

    str_copy(inode_table[idx].name, name, MAX_NAME_LEN);
    inode_table[idx].is_used = 1;
    inode_table[idx].is_dir  = 1;
    inode_table[idx].size    = 0;
    inode_table[idx].data    = NULL;
    inode_table[idx].parent  = superblock.current_dir;

    superblock.total_dirs++;
    return idx;
}

int vfs_write(const char *name, const char *content)
{
    int idx = vfs_find_by_name(name, superblock.current_dir);
    if (idx < 0) {
        /* Auto-create the file */
        idx = vfs_touch(name);
        if (idx < 0) return -1;
    }

    if (inode_table[idx].is_dir) {
        scr_print("  Error: '");
        scr_print(name);
        scr_print("' is a directory\n");
        return -1;
    }

    /* Free old data if any */
    if (inode_table[idx].data) {
        mem_free(inode_table[idx].data);
        inode_table[idx].data = NULL;
    }

    int content_len = str_length(content);
    if (content_len == 0) {
        inode_table[idx].size = 0;
        return 0;
    }

    /* Allocate new data block via memory.c */
    char *block = (char *)mem_alloc((size_t)(content_len + 1));
    if (!block) {
        scr_print("  Error: out of memory\n");
        return -1;
    }

    str_copy(block, content, content_len + 1);
    inode_table[idx].data = block;
    inode_table[idx].size = content_len;

    return 0;
}

void vfs_read(const char *name)
{
    int idx = vfs_find_by_name(name, superblock.current_dir);
    if (idx < 0) {
        scr_print("  Error: file '");
        scr_print(name);
        scr_print("' not found\n");
        return;
    }

    if (inode_table[idx].is_dir) {
        scr_print("  Error: '");
        scr_print(name);
        scr_print("' is a directory\n");
        return;
    }

    if (inode_table[idx].data && inode_table[idx].size > 0) {
        scr_print("  ");
        scr_print(inode_table[idx].data);
        scr_print("\n");
    } else {
        scr_print("  (empty file)\n");
    }
}

void vfs_rm(const char *name)
{
    int idx = vfs_find_by_name(name, superblock.current_dir);
    if (idx < 0) {
        scr_print("  Error: '");
        scr_print(name);
        scr_print("' not found\n");
        return;
    }

    /* If directory, check if empty */
    if (inode_table[idx].is_dir) {
        for (int i = 0; i < MAX_INODES; i++) {
            if (inode_table[i].is_used && inode_table[i].parent == idx) {
                scr_print("  Error: directory '");
                scr_print(name);
                scr_print("' is not empty\n");
                return;
            }
        }
        superblock.total_dirs--;
    } else {
        superblock.total_files--;
    }

    /* Free data via memory.c */
    if (inode_table[idx].data) {
        mem_free(inode_table[idx].data);
    }

    inode_table[idx].is_used = 0;
    inode_table[idx].data    = NULL;
    inode_table[idx].size    = 0;
    inode_table[idx].name[0] = '\0';

    scr_print("  Removed '");
    scr_print(name);
    scr_print("'\n");
}

void vfs_ls(void)
{
    int count = 0;
    scr_print("\n");

    for (int i = 0; i < MAX_INODES; i++) {
        if (inode_table[i].is_used &&
            inode_table[i].parent == superblock.current_dir) {

            char size_buf[16];
            str_itoa(inode_table[i].size, size_buf, 16);

            if (inode_table[i].is_dir) {
                scr_print("  \033[34m");
                scr_print(inode_table[i].name);
                scr_print("/\033[0m\n");
            } else {
                scr_print("  ");
                scr_print(inode_table[i].name);
                scr_print("  ");
                scr_print(size_buf);
                scr_print(" B\n");
            }
            count++;
        }
    }

    if (count == 0) {
        scr_print("  (empty directory)\n");
    }

    char count_buf[16];
    str_itoa(count, count_buf, 16);
    scr_print("\n  ");
    scr_print(count_buf);
    scr_print(" item(s)\n");
}

void vfs_cd(const char *name)
{
    if (!name || str_length(name) == 0 || str_compare(name, "/") == 0) {
        superblock.current_dir = 0;
        return;
    }

    if (str_compare(name, "..") == 0) {
        int parent = inode_table[superblock.current_dir].parent;
        if (parent >= 0) {
            superblock.current_dir = parent;
        }
        return;
    }

    int idx = vfs_find_by_name(name, superblock.current_dir);
    if (idx < 0) {
        scr_print("  Error: directory '");
        scr_print(name);
        scr_print("' not found\n");
        return;
    }

    if (!inode_table[idx].is_dir) {
        scr_print("  Error: '");
        scr_print(name);
        scr_print("' is not a directory\n");
        return;
    }

    superblock.current_dir = idx;
}

void vfs_get_current_path(char *buf, int buf_size)
{
    if (superblock.current_dir == 0) {
        str_copy(buf, "/", buf_size);
        return;
    }

    /* Build path by traversing parent chain */
    char parts[8][MAX_NAME_LEN];
    int depth = 0;
    int cur = superblock.current_dir;

    while (cur > 0 && depth < 8) {
        str_copy(parts[depth], inode_table[cur].name, MAX_NAME_LEN);
        depth++;
        cur = inode_table[cur].parent;
    }

    buf[0] = '\0';
    for (int i = depth - 1; i >= 0; i--) {
        str_concat(buf, "/", buf_size);
        str_concat(buf, parts[i], buf_size);
    }
}

int vfs_get_total_files(void) { return superblock.total_files; }
int vfs_get_total_dirs(void) { return superblock.total_dirs; }

int vfs_read_to_buf(const char *name, char *buf, int buf_size)
{
    if (!name || !buf || buf_size <= 0) return -1;

    int idx = vfs_find_by_name(name, superblock.current_dir);
    if (idx < 0) return -1;
    if (inode_table[idx].is_dir) return -1;

    if (!inode_table[idx].data || inode_table[idx].size == 0) {
        buf[0] = '\0';
        return 0;
    }

    str_copy(buf, inode_table[idx].data, buf_size);
    return inode_table[idx].size;
}
