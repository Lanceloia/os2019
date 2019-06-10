#include <klib.h>

#include "../../../include/fs/ext2fs.h"
#include "../../../include/fs/vfs.h"

#define MAX_FS 4

fs_t _fs[MAX_FS];
fs_ops_t _fs_ops[MAX_FS];

typedef struct device device_t;
extern device_t *dev_lookup(const char *name);

typedef struct ext2 ext2_t;
extern void ext2_init(fs_t *fs, const char *name, device_t *dev);
extern id_t *ext2_lookup(fs_t *fs, const char *path, int flags);
extern int ext2_close(id_t *id);

void vfs_build(int idx, char *name, device_t *dev, size_t size,
               void (*init)(fs_t *, const char *, device_t *),
               id_t *(*lookup)(fs_t *fs, const char *path, int flags),
               int (*close)(id_t *id)) {
  strcpy(_fs[idx].name, name);
  // printf("name: %s", name);
  _fs[idx].fs = pmm->alloc(size);
  _fs[idx].ops = &_fs_ops[idx];
  _fs[idx].dev = dev;
  _fs_ops[idx].init = init;
  _fs_ops[idx].lookup = lookup;
  _fs_ops[idx].close = close;
  _fs_ops[idx].init(&_fs[idx], name, dev);
}

void vfs_init() {
  vfs_build(0, "ext2fs-ramdisk0", dev_lookup("ramdisk0"), sizeof(ext2_t),
            ext2_init, ext2_lookup, ext2_close);
  // vfs_build(1, "ext2fs-ramdisk1", dev_lookup("ramdisk1"));
}

int vfs_access(const char *path, int mode) {
  assert(0);
  return 0;
}

int vfs_mount(const char *path, fs_t *fs) {
  assert(0);
  return 0;
}

int vfs_unmount(const char *path) {
  assert(0);
  return 1;
}

int vfs_mkdir(const char *path) {
  assert(0);
  return 1;
}

int vfs_rmdir(const char *path) {
  assert(0);
  return 1;
}

int vfs_link(const char *oldpath, const char *newpath) {
  assert(0);
  return 1;
}

int vfs_unlink(const char *path) {
  assert(0);
  return 1;
}

int vfs_open(const char *path, int flags) {
  assert(0);
  return 1;
}

ssize_t vfs_read(int fd, void *buf, size_t nbyte) {
  assert(0);
  return 1;
}

ssize_t vfs_write(int fd, void *buf, size_t nbyte) {
  assert(0);
  return 1;
}

off_t vfs_lseek(int fd, off_t offset, int whence) {
  assert(0);
  return 1;
}

int vfs_close(int fd) {
  assert(0);
  return 1;
}

fs_t *vfs_get_fs(int idx) {
  if (idx < 0 || idx >= MAX_FS)
    return NULL;
  else
    return &_fs[idx];
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
    .get_fs = vfs_get_fs,
};

#ifdef _LANCELOIA_DEBUG_
#undef _LANCELOIA_DEBUG_
#endif