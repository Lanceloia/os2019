#include <klib.h>

#include "../../include/fs/ext2fs.h"
#include "../../include/fs/vfs.h"

#define MAX_VINODE 1024
#define MAX_FILESYSTEM 16
#define VFS_ROOT 0
filesystem_t filesys[MAX_FILESYSTEM];
vinode_t vinodes[MAX_VINODE];

typedef struct device device_t;
extern device_t *dev_lookup(const char *name);

// helper
static int vinodes_alloc();
static void vinodes_free(int idx);
static int lookup_cur(char *path, int *flag, int cur);
static int lookup_root(char *path, int *flag);
static int lookup_auto(char *path);

static int vinodes_alloc() {
  for (int i = 0; i < MAX_VINODE; i++)
    if (vinodes[i].mode == UNUSED) return i;
  return -1;
}

static void vinodes_free(int idx) { vinodes[idx].mode = UNUSED; }

static int first_item_namelen(const char *path) {
  int ret = 0;
  for (; path[ret] && path[ret] != '/';) ret++;
  return ret + 1;
}

static int lookup_cur(char *path, int *pflag, int cur) {
  if (!strlen(path)) {
    *pflag = 1;
    return cur;
  }

  int k, len = first_item_namelen(path);
  for (k = vinodes[cur].child; k != -1; k = vinodes[k].next) {
    //  printf("%s  %d\n", vinodes[k].name, len);
    if (!strncmp(vinodes[k].name, path, len)) break;
  }

  if (k == -1) {
    *pflag = 0;
    return cur;
  }

  char *newpath = path + strlen(vinodes[k].name);
  // printf("old: %s, new: %s, name: %s\n", path, newpath, vinodes[k].name);
  return lookup_cur(newpath, pflag, k);
}

static int lookup_root(char *path, int *pflag) {
  return lookup_cur(path + 1, pflag, VFS_ROOT);
}

static int lookup_auto(char *path) {
  int len = strlen(path);
  if (path[len] != '/') strcat(path, "/");

  int flag;
  int idx = (path[0] == '/') ? lookup_root(path, &flag)
                             : lookup_cur(path, &flag, VFS_ROOT);

  if (flag == 1) return idx;

  int kth = 0, newidx = vinodes_alloc();
  while (vinodes[idx].fs->readdir(vinodes[idx].fs, vinodes[idx].rinode_idx,
                                  kth++, &vinodes[newidx].rinode_idx,
                                  vinodes[newidx].name))
    ;

  return lookup_auto(path);
}

static int filesys_alloc() {
  for (int i = 0; i < MAX_FILESYSTEM; i++)
    if (!strlen(filesys[i].name)) return i;
  return -1;
}

static void filesys_free(int idx) { strcpy(filesys[idx].name, ""); }

static void vfs_init_device(const char *name, device_t *dev, size_t size,
                            void (*init)(filesystem_t *, const char *,
                                         device_t *),
                            int (*lookup)(filesystem_t *, char *, int),
                            int (*readdir)(filesystem_t *, int, int, int *,
                                           char *)) {
  int idx = filesys_alloc();
  strcpy(filesys[idx].name, name);
  filesys[idx].rfs = pmm->alloc(size);
  filesys[idx].dev = dev;
  filesys[idx].init = init;
  filesys[idx].lookup = lookup;
  filesys[idx].readdir = readdir;
}

void vinodes_build(int idx, const char *name, char *path, int parent,
                   int mode) {
  strcpy(vinodes[idx].name, name);
  strcpy(vinodes[idx].path, path);
  vinodes[idx].dot = idx;
  vinodes[idx].ddot = parent;
  vinodes[idx].mode = mode;
  vinodes[idx].next = vinodes[idx].child = -1;
  vinodes[idx].prev_link = vinodes[idx].next_link = idx;
  vinodes[idx].linkcnt = 1;
}

void vinodes_mount(const char *name, int parent, int mode) {
  int idx = vinodes_alloc();
  strcpy(vinodes[idx].name, name);
  strcpy(vinodes[idx].path, vinodes[parent].path);
  strcat(vinodes[idx].path, name);
  int k = vinodes[parent].child;
  if (k == -1)
    vinodes[parent].child = idx;
  else {
    while (vinodes[k].next != -1) k = vinodes[k].next;
    vinodes[k].next = idx;
  }
  vinodes[idx].ddot = parent;
  vinodes[idx].mode = mode;
  vinodes[idx].next = vinodes[idx].child = -1;
  vinodes[idx].prev_link = vinodes[idx].next_link = idx;
  vinodes[idx].linkcnt = 1;
}

typedef struct ext2 ext2_t;
extern void ext2_init(filesystem_t *fs, const char *name, device_t *dev);
extern int ext2_lookup(filesystem_t *fs, char *path, int mode);
extern int ext2_readdir(filesystem_t *fs, int vinode_idx, int kth,
                        int *p_rinode_idx, char *namebuf);

int fuck() {
  lookup_auto("/");
  filesys_free(2);
  vinodes_alloc();
  vinodes_free(0);
  return 0;
}

int vfs_init() {
  vinodes_build(VFS_ROOT, "/", "/", VFS_ROOT, TYPE_DIR | RD_ABLE | WR_ABLE);
  vinodes_mount("dev/", VFS_ROOT, TYPE_DIR | RD_ABLE | WR_ABLE);
  vfs_init_device("ramdisk0", dev_lookup("ramdisk0"), sizeof(ext2_t), ext2_init,
                  ext2_lookup, ext2_readdir);
  /*
  strcpy(vfsdirs[0].name, "/");
  strcpy(vfsdirs[0].absolutely_name, "/");
  vfsdirs[0].dot = vfsdirs[0].ddot = 0;
  vfsdirs[0].next = vfsdirs[0].child = -1;
  vfsdirs[0].type = VFS;
  dev_dir = vfsdirs_alloc("dev", 0, VFS, -1);
  proc_dir = vfsdirs_alloc("proc", 0, PROCFS, -1);
  vfs_device(total_dev_cnt, "ext2fs", dev_lookup("ramdisk0"),
  sizeof(ext2_t), ext2_init, ext2_lookup_tmp, ext2_open_tmp, ext2_close_tmp,
             ext2_mkdir_tmp, ext2_rmdir_tmp);
  vfsdirs_alloc("ramdisk0", dev_dir, EXT2, total_dev_cnt++);

  vfs_device(total_dev_cnt, "ext2fs", dev_lookup("ramdisk1"),
  sizeof(ext2_t), ext2_init, ext2_lookup_tmp, ext2_open_tmp, ext2_close_tmp,
             ext2_mkdir_tmp, ext2_rmdir_tmp);
  vfsdirs_alloc("ramdisk1", dev_dir, EXT2, total_dev_cnt++);
  */

  printf("%s  %d\n", "/", lookup_auto("/"));
  printf("%s  %d\n", "/dev", lookup_auto("/dev"));
  printf("%s  %d\n", "/dev/", lookup_auto("/dev/"));
  return 0;
}

int vfs_access(const char *path, int mode) { return 0; }

int vfs_mount(const char *path, filesystem_t *fs) { return 0; }

int vfs_unmount(const char *path) { return 0; }

int vfs_mkdir(const char *path) { return 0; }

int vfs_rmdir(const char *path) { return 0; }

int vfs_link(const char *oldpath, const char *newpath) { return 0; }

int vfs_unlink(const char *path) { return 0; }

int vfs_open(const char *path, int flags) { return 0; }

ssize_t vfs_read(int fd, char *buf, size_t nbyte) { return 0; }

ssize_t vfs_write(int fd, char *buf, size_t nbyte) { return 0; }

off_t vfs_lseek(int fd, off_t offset, int whence) { return 0; }

int vfs_close(int fd) { return 0; }

void vfs_ls(int idx) {
  printf("       index       name        path\n");
  printf("cur:   %4d        %8s    %s\n", idx, vinodes[idx].name,
         vinodes[idx].path);
  for (int k = vinodes[idx].child; k != -1; k = vinodes[k].next) {
    printf("child: %4d        %8s    %s\n", k, vinodes[k].name,
           vinodes[k].path);
  }
}

/*
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
*/

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

id_t *ext2_lookup_tmp(fs_t *fs, const char *path, int flags) { return NULL;
} int ext2_open_tmp(id_t *id, int flags) { return 0; } int
ext2_close_tmp(id_t *id) { return 0; } int ext2_mkdir_tmp(const char
*dirname) { return 0; } int ext2_rmdir_tmp(const char *dirname) { return 0;
}
*/

/*
void vfs_device(int idx, char *name, device_t *dev, size_t size,
                void (*init)(fs_t *, const char *, device_t *),
                id_t *(*lookup)(fs_t *fs, const char *path, int flags),
                int (*open)(id_t *id, int flags), int (*close)(id_t *id),
                int (*mkdir)(const char *name),
                int (*rmdir)(const char *name)) {
  strcpy(filesys[idx].name, name);
  // printf("name: %s", name);
  filesys[idx].realfilesys = pmm->alloc(size);
  filesys[idx].ops = &filesys_ops[idx];
  filesys[idx].dev = dev;
  filesys_ops[idx].init = init;
  filesys_ops[idx].lookup = lookup;
  filesys_ops[idx].open = open;
  filesys_ops[idx].close = close;
  filesys_ops[idx].mkdir = mkdir;
  filesys_ops[idx].rmdir = rmdir;
  filesys_ops[idx].init(&filesys[idx], name, dev);
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
    vfsdirs[idx].realfilesys = filesys[fs_idx].realfilesys;
  else
    vfsdirs[idx].realfilesys = NULL;

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

/*
void vfs_init(){
  // vfs_device(1, "tty1", dev_lookup("tty1"));
  cur_dir = 0;
  strcpy(vfsdirs[0].name, "/");
  strcpy(vfsdirs[0].absolutely_name, "/");
  vfsdirs[0].dot = vfsdirs[0].ddot = 0;
  vfsdirs[0].next = vfsdirs[0].child = -1;
  vfsdirs[0].type = VFS;
  dev_dir = vfsdirs_alloc("dev", 0, VFS, -1);
  proc_dir = vfsdirs_alloc("proc", 0, PROCFS, -1);
  vfs_device(total_dev_cnt, "ext2fs", dev_lookup("ramdisk0"),
sizeof(ext2_t), ext2_init, ext2_lookup_tmp, ext2_open_tmp, ext2_close_tmp,
             ext2_mkdir_tmp, ext2_rmdir_tmp);
  vfsdirs_alloc("ramdisk0", dev_dir, EXT2, total_dev_cnt++);

  vfs_device(total_dev_cnt, "ext2fs", dev_lookup("ramdisk1"),
sizeof(ext2_t), ext2_init, ext2_lookup_tmp, ext2_open_tmp, ext2_close_tmp,
             ext2_mkdir_tmp, ext2_rmdir_tmp);
  vfsdirs_alloc("ramdisk1", dev_dir, EXT2, total_dev_cnt++);
}
*/