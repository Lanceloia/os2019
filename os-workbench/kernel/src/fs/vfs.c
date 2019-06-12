#include <klib.h>

#include "../../include/fs/ext2fs.h"
#include "../../include/fs/vfs.h"

#define MAX_FILESYSTEM 16

filesystem_t filesys[MAX_FILESYSTEM];

typedef struct device device_t;
extern device_t *dev_lookup(const char *name);

/*
typedef struct ext2 ext2_t;
extern void ext2_init(fs_t *fs, const char *name, device_t *dev);

extern id_t *ext2_lookup(fs_t *fs, const char *path, int flags);
extern int ext2_open(id_t *id, int flags);
extern int ext2_close(id_t *id);
extern int ext2_mkdir(const char *dirname);
extern int ext2_rmdir(const char *dirname);
*/

/*

id_t *ext2_lookup_tmp(fs_t *fs, const char *path, int flags) { return NULL; }
int ext2_open_tmp(id_t *id, int flags) { return 0; }
int ext2_close_tmp(id_t *id) { return 0; }
int ext2_mkdir_tmp(const char *dirname) { return 0; }
int ext2_rmdir_tmp(const char *dirname) { return 0; }
*/

/*
void vfs_device(int idx, char *name, device_t *dev, size_t size,
                void (*init)(fs_t *, const char *, device_t *),
                id_t *(*lookup)(fs_t *fs, const char *path, int flags),
                int (*open)(id_t *id, int flags), int (*close)(id_t *id),
                int (*mkdir)(const char *name),
                int (*rmdir)(const char *name)) {
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
*/

/*
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
*/

void vfs_init() {
  /*
  // vfs_device(1, "tty1", dev_lookup("tty1"));
  cur_dir = 0;
  strcpy(vfsdirs[0].name, "/");
  strcpy(vfsdirs[0].absolutely_name, "/");
  vfsdirs[0].dot = vfsdirs[0].ddot = 0;
  vfsdirs[0].next = vfsdirs[0].child = -1;
  vfsdirs[0].type = VFS;
  dev_dir = vfsdirs_alloc("dev", 0, VFS, -1);
  proc_dir = vfsdirs_alloc("proc", 0, PROCFS, -1);
  vfs_device(total_dev_cnt, "ext2fs", dev_lookup("ramdisk0"), sizeof(ext2_t),
             ext2_init, ext2_lookup_tmp, ext2_open_tmp, ext2_close_tmp,
             ext2_mkdir_tmp, ext2_rmdir_tmp);
  vfsdirs_alloc("ramdisk0", dev_dir, EXT2, total_dev_cnt++);

  vfs_device(total_dev_cnt, "ext2fs", dev_lookup("ramdisk1"), sizeof(ext2_t),
             ext2_init, ext2_lookup_tmp, ext2_open_tmp, ext2_close_tmp,
             ext2_mkdir_tmp, ext2_rmdir_tmp);
  vfsdirs_alloc("ramdisk1", dev_dir, EXT2, total_dev_cnt++);
  */
}

int vfs_access(const char *path, int mode) { return 0; }

int vfs_mount(const char *path, filesystem_t *fs) { return 0; }

int vfs_unmount(const char *path) { return 0; }

int vfs_mkdir(const char *path) { return 0; }

int vfs_rmdir(const char *path) { return 0; }

int vfs_link(const char *oldpath, const char *newpath) { return 0; }

int vfs_unlink(const char *path) { return 0; }

int vfs_open(const char *path, int flags) { return 0; }

ssize_t vfs_read(int fd, void *buf, size_t nbyte) { return 0; }

ssize_t vfs_write(int fd, void *buf, size_t nbyte) { return 0; }

off_t vfs_lseek(int fd, off_t offset, int whence) { return 0; }

int vfs_close(int fd) { return 0; }

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

* /