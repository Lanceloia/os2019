#include <common.h>
#include <klib.h>
#include <fs.h>

void vfs_init() {}

int vfs_access(const char *path, int mode)
{
    return 1;
}

int vfs_mount(const char *path, filesystem_t *fs)
{
    return 1;
}

int vfs_unmount(const char *path)
{
    return 1;
}

int vfs_mkdir(const char *path)
{
    return 1;
}

int vfs_rmdir(const char *path)
{
    return 1;
}

int vfs_link(const char *oldpath, const char *newpath)
{
    return 1;
}

int vfs_unlink(const char *path)
{
    return 1;
}

int vfs_open(const char *path, int flags)
{
    return 1;
}

ssize_t vfs_read(int fd, void *buf, size_t nbyte)
{
    return 1;
}

ssize_t vfs_write(int fd, void *buf, size_t nbyte)
{
    return 1;
}

off_t vfs_lseek(int fd, off_t offset, int whence)
{
    return 1;
}

int vfs_close(int fd)
{
    return 1;
}

MODULE_DEF(vfs){
    .init = vfs_init,
    .access = vfs_access,
    .mount = vfs_mount,
    .unmount = vfs_unmount,
    .mkdir = vfs_mkdir,
    .rmdir = vfs_rmdir,
    .link = vfs_link,
    .unlink = vfs_unlink,
    .open = vfs_open,
    .read = vfs_read,
    .write = vfs_write,
    .lseek = vfs_lseek,
    .close = vfs_close,
};

#ifdef _LANCELOIA_DEBUG_
#undef _LANCELOIA_DEBUG_
#endif