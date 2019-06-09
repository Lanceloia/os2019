#ifndef __FS_H__
#define __FS_H__

enum
{
    RD_ENABLE = 1,
    WR_ENABLE = 2,
};

typedef struct file file_t;
typedef struct inodeops inodeops_t;
typedef struct inode inode_t;
typedef struct filesystem filesystem_t;
typedef struct fsops fsops_t;

struct file
{
    int refcnt;
    inode_t *inode;
    uint64_t offset;
};

struct inode
{
    int refcnt;
    void *prt; //??
    filesystem_t *fs;
    inodeops_t *ops;
};

struct inodeops
{
    int (*open)(file_t *file, int flags);
    int (*close)(file_t *file);
    ssize_t (*read)(file_t *file, char *buf, size_t size);
    ssize_t (*write)(file_t *file, const char *buf, size_t size);
    off_t (*lseek)(file_t *file, off_t offset, int whence);
    int (*mkdir)(const char *name);
    int (*rmdir)(const char *name);
    int (*link)(const char *name, inode_t *inode);
    int (*unlink)(const char *name);
};

struct filesystem
{
    char name[NAME_lENGTH];
    fsops_t *ops;
    device_t *dev;
};

struct fsops
{
    void (*init)(filesystem_t *fs, const char *name, device_t *dev);
    inode_t *(lookup)(filesystem_t *fs, const char *path, int flags);
    int (*close)(inode_t *inode);
};

#endif