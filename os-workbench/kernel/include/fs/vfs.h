#ifndef __FS_H__
#define __FS_H__

#include <common.h>

#define EX_ABLE = 0x0001
#define WR_ABLE = 0x0002
#define RD_ABLE = 0x0004
#define TYPE_FILE = 0x0100
#define TYPE_DIR = 0x0200
#define TYPE_LINK = 0x0400

#define MAX_PATH_LENGTH 256
#define MAX_NAME_LENGTH 32

typedef struct filesystem filesystem_t;

typedef struct file {
  int refcnt;       // no used
  int idx;          // vinode index
  uint64_t offset;  // read or write offset
} file_t;

typedef struct vinode {
  char path[MAX_PATH_LENGTH];  // the path from VFS_ROOT
  char name[MAX_NAME_LENGTH];  // the name of vinode
  int dot, ddot;               // current idx, parent idx
  int next, child;             // next brother idx, first child idx

  int mode;          // TYPE, RWX_MODE
  int linkcnt;       // link cnt
  filesystem_t *fs;  // filesystem pointer

  int prev_link, next_link;  // prev or next link

  // file operation
  int (*open)(int vinode_idx, int mode);
  int (*close)(int fd);
  ssize_t (*read)(int fd, char *buf, size_t size);
  ssize_t (*write)(int fd, const char *buf, size_t size);
  off_t (*lseek)(int fd, off_t offset, int whence);

  // vinode operation
  int (*add_link)(int old_vinode_idx, int new_vinode_idx);
  int (*rm_link)(int vinode_idx);
  int (*mkdir)(int vinode, const char *name);
  int (*rmdir)(int vinode, const char *name);
} vinode_t;

struct filesystem {
  char name[NAME_lENGTH];
  void *rfs;
  device_t *dev;

  void (*init)(filesystem_t *fs, const char *name, device_t *dev);
  int (*lookup)(filesystem_t *fs, const char *path, int mode);
  int (*close)(int vinode_idx);
};

// helper
int vinode_alloc();
void vinode_free(int idx);
int lookup_cur(const char *path, int *flag, int cur);
int lookup_root(const char *path, int *flag);
int lookup_auto(const char *path);

// interface
int vfs_init();
int vfs_access(const char *path, int mode);
int vfs_mount(const char *path, filesystem_t *fs);
int vfs_unmount(const char *path);
int vfs_mkdir(const char *path);
int vfs_rmdir(const char *path);
int vfs_link(const char *oldpath, const char *newpath);
int vfs_open(const char *path, int mode);
ssize_t vfs_read(int fd, char *buf, size_t nbyte);
ssize_t vfs_write(int fs, char *buf, size_t nbyte);
int vfs_close(int fd);

#endif