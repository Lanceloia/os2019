#ifndef __FS_H__
#define __FS_H__

enum {
  RD_ENABLE = 1,
  WR_ENABLE = 2,
};

typedef struct file_desc fd_t;
typedef struct inode_desc id_t;
typedef struct inode_desc_ops id_ops_t;
typedef struct file_sys fs_t;
typedef struct file_sys_ops fs_ops_t;

struct file_desc {
  int refcnt;
  id_t *id;
  uint64_t offset;
};

struct inode_desc {
  uint32_t refcnt;
  void *ptr;  // private data start
  fs_t *fs;
  id_ops_t *ops;
};

struct inode_desc_ops {
  int (*open)(fd_t *file, int flags);
  int (*close)(fd_t *file);
  ssize_t (*read)(fd_t *file, char *buf, size_t size);
  ssize_t (*write)(fd_t *file, const char *buf, size_t size);
  off_t (*lseek)(fd_t *file, off_t offset, int whence);
  int (*mkdir)(const char *name);
  int (*rmdir)(const char *name);
  int (*link)(const char *name, id_t *id);
  int (*unlink)(const char *name);
};

struct file_sys {
  char name[NAME_lENGTH];
  void *fs;
  fs_ops_t *ops;
  device_t *dev;
};

struct file_sys_ops {
  void (*init)(fs_t *fs, const char *name, device_t *dev);
  id_t (*lookup)(fs_t *fs, const char *path, int flags);
  int (*close)(id_t *id);
};

#endif