#ifndef __FS_H__
#define __FS_H__

#include <common.h>

#define UNUSED 0x00
#define EX_ABLE 0x01
#define WR_ABLE 0x02
#define RD_ABLE 0x04
#define TYPE_FILE 0x10
#define TYPE_DIR 0x20
#define TYPE_LINK 0x40
#define ALLOCED 0x80

#define VFS 0x00
#define EXT2FS 0x01
#define PROCFS 0x02

#define MAX_PATH_LENGTH 256
#define MAX_NAME_LENGTH 32

#define EXT2_ROOT 1

typedef struct filesystem filesystem_t;

typedef struct file {
  int refcnt;       // no used
  int vinode_idx;   // vinode index
  uint64_t offset;  // read or write offset
} file_t;

typedef struct vinode {
  char path[MAX_PATH_LENGTH];  // the path from VFS_ROOT
  char name[MAX_NAME_LENGTH];  // the name of vinode
  int dot, ddot;               // current idx, parent idx
  int next, child;             // next brother idx, first child idx

  int mode;          // TYPE, RWX_MODE
  int linkcnt;       // link cnt
  int ridx;          // read inode idx
  int fs_type;       // filesystem type
  filesystem_t *fs;  // filesystem pointer

  int prev_link, next_link;  // prev or next link
} vinode_t;

// file operation
ssize_t vinode_read(int fd, char *buf, ssize_t size);
ssize_t vinode_write(int fd, char *buf, ssize_t size);
off_t vinode_lseek(int fd, off_t offset, int whence);

// vinode operation
void add_link(int oidx, int nidx);
void remove_link(int idx);
int vinode_rm_link(int vinode_idx);
int vinode_mkdir(int vinode, const char *name);
int vinode_rmdir(int vinode, const char *name);

struct filesystem {
  char name[NAME_lENGTH];
  void *rfs;
  device_t *dev;
  void (*init)(filesystem_t *fs, const char *name, device_t *dev);
  int (*lookup)(filesystem_t *fs, char *path, int mode);
  int (*readdir)(filesystem_t *fs, int vinode_idx, int kth, vinode_t *buf);
};

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