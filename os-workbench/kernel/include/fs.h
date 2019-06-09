#ifndef __FS_H__
#define __FS_H__

enum {
    RD_ENABLE = 1,
    WR_ENABLE = 2,
};

static int vfs_access(const char *path, int mode);
static int vfs_mount(const char *path, filesystem_t *fs);
static int vfs_unmount(const char *path);
static int vfs_mkdir(const char *path);
static int vfs_rmdir(const char *path);
static int vfs_link(const char *oldpath, const char *newpath);
static int vfs_unlink(const char *path);
static int vfs_open(const char *path, int flags);
static ssize_t vfs_read(int fd, void *buf, size_t nbyte);
static ssize_t vfs_write(int fd, void *buf, size_t nbyte);
static off_t vfs_lseek(int fd, off_t offset, int whence);
static int vfs_close(int fd);

#endif