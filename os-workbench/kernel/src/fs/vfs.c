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
/*
extern id_t *ext2_lookup(fs_t *fs, const char *path, int flags);
extern int ext2_open(id_t *id, int flags);
extern int ext2_close(id_t *id);
extern int ext2_mkdir(const char *dirname);
extern int ext2_rmdir(const char *dirname);
*/

id_t *ext2_lookup(fs_t *fs, const char *path, int flags) { return NULL; }
int ext2_open(id_t *id, int flags) { return 0; }
int ext2_close(id_t *id) { return 0; }
int ext2_mkdir_tmp(const char *dirname) { return 0; }
int ext2_rmdir_tmp(const char *dirname) { return 0; }

void vfs_build(int idx, char *name, device_t *dev, size_t size,
               void (*init)(fs_t *, const char *, device_t *),
               id_t *(*lookup)(fs_t *fs, const char *path, int flags),
               int (*open)(id_t *id, int flags), int (*close)(id_t *id),
               int (*mkdir)(const char *name), int (*rmdir)(const char *name)) {
  strcpy(_fs[idx].name, name);
  // printf("name: %s", name);
  _fs[idx].real_fs = pmm->alloc(size);
  _fs[idx].ops = &_fs_ops[idx];
  _fs[idx].dev = dev;
  _fs_ops[idx].init = init;
  _fs_ops[idx].lookup = lookup;
  _fs_ops[idx].open = open;
  _fs_ops[idx].close = close;
  _fs_ops[idx].mkdir = mkdir;
  _fs_ops[idx].rmdir = rmdir;
  _fs_ops[idx].init(&_fs[idx], name, dev);
}

#define MAX_DIRS 256

struct vfsdir {
  char name[16];
  char absolutely_name[256];
  int dot, ddot;
  int next, child, type;
  void *real_fs;
} vfsdirs[MAX_DIRS] = {};
int cur_dir, dev_dir, proc_dir;

int vfsdirs_alloc(const char *name, int parent, int type, int fs_idx) {
  int idx = -1;
  for (int i = 0; idx == -1 && i < MAX_DIRS; i++)
    if (strlen(vfsdirs[i].name) == 0) idx = i;
  if (idx == -1) assert(0);
  strcpy(vfsdirs[idx].name, name);
  strcpy(vfsdirs[idx].absolutely_name, vfsdirs[parent].absolutely_name);
  strcat(vfsdirs[idx].absolutely_name, name);
  strcat(vfsdirs[idx].absolutely_name, "/");
  vfsdirs[idx].dot = idx;
  vfsdirs[idx].ddot = parent;
  vfsdirs[idx].next = -1;
  vfsdirs[idx].child = -1;
  vfsdirs[idx].type = type;
  if (fs_idx >= 0)
    vfsdirs[idx].real_fs = _fs[fs_idx].real_fs;
  else
    vfsdirs[idx].real_fs = NULL;

  if (vfsdirs[parent].child == -1)
    vfsdirs[parent].child = idx;
  else {
    for (int i = vfsdirs[parent].child;; i = vfsdirs[i].next) {
      if (vfsdirs[i].next == -1) {
        vfsdirs[i].next = idx;
        break;
      }
    }
  }
  return idx;
}

void vfs_init() {
  // vfs_build(1, "tty1", dev_lookup("tty1"));
  cur_dir = 0;
  strcpy(vfsdirs[0].name, "/");
  strcpy(vfsdirs[0].absolutely_name, "/");
  vfsdirs[0].dot = vfsdirs[0].ddot = 0;
  vfsdirs[0].next = vfsdirs[0].child = -1;
  vfsdirs[0].type = VFS;
  dev_dir = vfsdirs_alloc("dev", 0, VFS, -1);
  proc_dir = vfsdirs_alloc("proc", 0, VFS, -1);
  vfs_build(0, "ext2fs-ramdisk0", dev_lookup("ramdisk0"), sizeof(ext2_t),
            ext2_init, ext2_lookup, ext2_open, ext2_close, ext2_mkdir_tmp,
            ext2_rmdir_tmp);
  vfsdirs_alloc("ramdisk0", dev_dir, EXT2, 0);

  vfs_build(1, "ext2fs-ramdisk1", dev_lookup("ramdisk1"), sizeof(ext2_t),
            ext2_init, ext2_lookup, ext2_open, ext2_close, ext2_mkdir_tmp,
            ext2_rmdir_tmp);
  vfsdirs_alloc("ramdisk1", dev_dir, EXT2, 1);
}

/*
void vfsdirs_dfs(const char *path){

}
*/

int vfs_match_path(const char *path) {
  int idx = -1, len = 0;
  for (int i = 0; i < MAX_DIRS; i++) {
    int slen = strlen(vfsdirs[i].absolutely_name);
    if (!strncmp(path, vfsdirs[i].absolutely_name, slen))
      if (slen > len) {
        idx = i;
        len = slen;
      }
  }
  return idx;
}

int vfs_identify_fs(const char *path) {
  int idx = vfs_match_path(path);
  if (idx == -1) {
    printf("unknown filesystem.\n");
    return 0;
  }
  int type = vfsdirs[idx].type;
  if (!strcmp(path, vfsdirs[idx].absolutely_name))  // absolutely_equal
    return type | INTERFACE;
  else
    return type;
}

void *vfs_get_realfs(const char *path) {
  int idx = vfs_match_path(path);
  if (idx == -1) {
    printf("unknown filesystem.\n");
    return NULL;
  }
  return vfsdirs[idx].real_fs;
}

int vfs_access(const char *path, int mode) {
  /*
  int idx = vfs_identify_fs(path);
  // 0 for accessable
  if (_fs[idx].ops->lookup(&_fs[idx], path + strlen(_path[idx]), mode) ==
  NULL) return 1;
  */
  return 0;
}

int vfs_mount(const char *path, fs_t *fs) {
  /*
  int idx = fs - &_fs[0];
  if (idx < 0 || idx >= MAX_FS) return 1;
  strcpy(_path[idx], path);
  */
  return 0;
}

int vfs_unmount(const char *path) {
  /*
  int idx = vfs_identify_fs(path);
  _path[idx] = "DISABLE_PATH";
  */
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

/*
static int alloc_fd() {
  for (int i = 0; i < MAX_FD; i++)
    if (fds[i].refcnt == 0) return i;
  return -1;
}
*/

int vfs_open(const char *path, int flags) {
  /*
  assert(0);
  if (vfs_access(path, flags)) return 1;
  int fd = alloc_fd();
  if (fd == -1) return 1;
  int idx = vfs_identify_fs(path);
  fds[fd].id =
      _fs[idx].ops->lookup(&_fs[idx], path + strlen(_path[idx]), flags);
  fds[fd].offset = 0;
  fds[fd].refcnt++;
  */
  return 0;
}

ssize_t vfs_read(int fd, void *buf, size_t nbyte) {
  /*
  ssize_t cnt = fds[fd].id->ops->read(&fds[fd], buf, nbyte);
  fds[fd].offset += cnt;
  */
  return 0;
}

ssize_t vfs_write(int fd, void *buf, size_t nbyte) {
  /*
  assert(0);
  ssize_t cnt = fds[fd].id->ops->write(&fds[fd], buf, nbyte);
  fds[fd].offset += cnt;
  */
  return 0;
}

off_t vfs_lseek(int fd, off_t offset, int whence) {
  assert(0);
  return 1;
}

int vfs_close(int fd) {
  /*
  if (!(--fds[fd].refcnt)) {
    fds[fd].id = NULL;
    fds[fd].offset = 0;
  }
  */
  return 0;
}

void vfs_cd(char *dirname, char *pwd, char *out) {
  int offset = sprintf(out, "");
  if (!strcmp(dirname, "."))
    cur_dir = vfsdirs[cur_dir].dot;
  else if (!strcmp(dirname, ".."))
    cur_dir = vfsdirs[cur_dir].ddot;
  else {
    for (int i = vfsdirs[cur_dir].child; i != -1; i = vfsdirs[i].next) {
      if (!strcmp(dirname, vfsdirs[i].name)) cur_dir = i, i = -1;
    }
  }
  strcpy(pwd, vfsdirs[cur_dir].absolutely_name);
  offset += sprintf(out + offset, "Current directory: %s\n", pwd);
}

void vfs_ls(char *dirname, char *pwd, char *out) {
  int offset = sprintf(out, ""), tmp_dir = -1;
  if (!strcmp(dirname, "."))
    tmp_dir = vfsdirs[cur_dir].dot;
  else if (!strcmp(dirname, ".."))
    tmp_dir = vfsdirs[cur_dir].ddot;
  else {
    for (int i = vfsdirs[cur_dir].child; i != -1; i = vfsdirs[i].next) {
      if (!strcmp(dirname, vfsdirs[i].name)) tmp_dir = i, i = -1;
    }
  }
  if (tmp_dir == -1)
    offset += sprintf(out + offset, "No such directory.\n");
  else {
    offset += sprintf(out + offset, "items           type     path\n");
    for (int i = vfsdirs[tmp_dir].child; i != -1; i = vfsdirs[i].next) {
      offset += sprintf(out + offset, "%s", vfsdirs[i].name);
      for (int j = 0; j < 15 - strlen(vfsdirs[i].name); j++)
        offset += sprintf(out + offset, "%c", ' ');
      offset += sprintf(out + offset, " <DIR>    ");
      offset += sprintf(out + offset, "%s", vfsdirs[i].absolutely_name);
      offset += sprintf(out + offset, "\n");
    }
  }
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
    //.get_fs = vfs_get_fs,
};

#ifdef _LANCELOIA_DEBUG_
#undef _LANCELOIA_DEBUG_
#endif