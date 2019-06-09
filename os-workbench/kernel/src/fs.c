#include <common.h>
#include <fs.h>

void vfs_init();
int vfs_access(const char *path, int mode);
int vfs_mount(const char *path, filesystem_t *fs);
int vfs_unmount(const char *path);
int vfs_mkdir(const char *path);
int vfs_rmdir(const char *path);
int vfs_link(const char *oldpath, const char *newpath);
int vfs_unlink(const char *path);
int vfs_open(const char *path, int flags);
ssize_t vfs_read(int fd, void *buf, size_t nbyte);
ssize_t vfs_write(int fd, void *buf, size_t nbyte);
off_t vfs_lseek(int fd, off_t offset, int whence);
int vfs_close(int fd);

MODULE_DEF(vfs){
    .init = vfs_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
};

#ifdef _LANCELOIA_DEBUG_
#undef _LANCELOIA_DEBUG_
#endif