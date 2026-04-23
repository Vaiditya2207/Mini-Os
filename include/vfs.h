#ifndef VFS_H
#define VFS_H

#define MAX_INODES     64
#define MAX_NAME_LEN   32
#define MAX_DATA_SIZE  1024

void vfs_init(void);
int  vfs_touch(const char *name);
int  vfs_mkdir(const char *name);
int  vfs_write(const char *name, const char *content);
void vfs_read(const char *name);
void vfs_rm(const char *name);
void vfs_ls(void);
void vfs_cd(const char *name);
void vfs_get_current_path(char *buf, int buf_size);

/* Expose for sysinfo */
int vfs_get_total_files(void);
int vfs_get_total_dirs(void);

/* Read raw file content into caller-supplied buffer (for script engine) */
int vfs_read_to_buf(const char *name, char *buf, int buf_size);

#endif /* VFS_H */
