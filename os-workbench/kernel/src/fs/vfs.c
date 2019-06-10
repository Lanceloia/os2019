#include <klib.h>

#include "../../include/fs/ext2fs.h"
#include "../../include/fs/vfs.h"

#define MAX_FS 4

fs_t _fs[MAX_FS];
fs_ops_t _fs_ops[MAX_FS];
char *_path[MAX_FS];

typedef struct device device_t;
extern device_t *dev_lookup(const char *name);

typedef struct ext2 ext2_t;
extern void ext2_init(fs_t *fs, const char *name, device_t *dev);
extern id_t *ext2_lookup(fs_t *fs, const char *path, int flags);
extern int ext2_close(id_t *id);

void vfs_build(int idx, char *name, char *path, device_t *dev, size_t size,
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
  _path[idx] = path;
}

#define MAX_FD 32

fd_t fds[MAX_FD];
id_t ids[32];

struct node {
  char *name;
  int dot, ddot;
  int next, child;
} nodes[256] = {
    {"/", 0, 0, -1, 1}, {"/dev/", 1, 0, 2, -1}, {"/proc/", 2, 0, -1, -1}};

void vfs_init() {
  vfs_build(0, "ext2fs-ramdisk0", "/dev/ramdisk0", dev_lookup("ramdisk0"),
            sizeof(ext2_t), ext2_init, ext2_lookup, ext2_close);
  // vfs_build(1, "tty1", dev_lookup("tty1"));
}

int vfs_identify_fs(const char *path) {
  int idx = -1;
  for (int i = 0; i < MAX_FS; i++)
    if (!strncmp(path, _path[i], strlen(_path[i]))) idx = i;
  if (idx == -1) {
    printf("unknown filesystem.\n");
    return -1;
  }
  return idx;
}

int vfs_access(const char *path, int mode) {
  int idx = vfs_identify_fs(path);
  // 0 for accessable
  if (_fs[idx].ops->lookup(&_fs[idx], path + strlen(_path[idx]), mode) == NULL)
    return 1;
  return 0;
}

int vfs_mount(const char *path, fs_t *fs) {
  int idx = fs - &_fs[0];
  if (idx < 0 || idx >= MAX_FS) return 1;
  strcpy(_path[idx], path);
  return 0;
}

int vfs_unmount(const char *path) {
  int idx = vfs_identify_fs(path);
  _path[idx] = "DISABLE_PATH";
  return 0;
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

static int alloc_fd() {
  for (int i = 0; i < MAX_FD; i++)
    if (fds[i].refcnt == 0) return i;
  return -1;
}

int vfs_open(const char *path, int flags) {
  assert(0);
  if (vfs_access(path, flags)) return 1;
  int fd = alloc_fd();
  if (fd == -1) return 1;
  int idx = vfs_identify_fs(path);
  fds[fd].id =
      _fs[idx].ops->lookup(&_fs[idx], path + strlen(_path[idx]), flags);
  fds[fd].offset = 0;
  fds[fd].refcnt++;
  return 0;
}

ssize_t vfs_read(int fd, void *buf, size_t nbyte) {
  ssize_t cnt = fds[fd].id->ops->read(&fds[fd], buf, nbyte);
  fds[fd].offset += cnt;
  return cnt;
}

ssize_t vfs_write(int fd, void *buf, size_t nbyte) {
  assert(0);
  ssize_t cnt = fds[fd].id->ops->write(&fds[fd], buf, nbyte);
  fds[fd].offset += cnt;
  return cnt;
}

off_t vfs_lseek(int fd, off_t offset, int whence) {
  assert(0);
  return 1;
}

int vfs_close(int fd) {
  if (!(--fds[fd].refcnt)) {
    fds[fd].id = NULL;
    fds[fd].offset = 0;
  }
  return 0;
}

fs_t *vfs_get_fs(int idx) {
  if (idx < 0 || idx >= MAX_FS)
    return NULL;
  else
    return &_fs[idx];
}

void vfs_cd(char *dirname, char *pwd, char *out) {
  int offset = sprintf(out, "");
  if (!strcmp(dirname, "../")) dirname[2] = '\0';
  if (!strcmp(dirname, "./")) dirname[1] = '\0';

  int idx = 0;
  for (; strcmp(nodes[idx].name, pwd);) idx++;

  if (!strcmp(dirname, ".."))
    strcpy(pwd, nodes[nodes[idx].ddot].name);
  else if (!strcmp(dirname, "."))
    ;
  else {
    strcat(pwd, dirname);
    strcat(pwd, "/");
  }
  offset += sprintf(out + offset, "Current directory: %s\n", pwd);
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