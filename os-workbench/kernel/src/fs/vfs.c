#include <klib.h>

#include "../../include/fs/ext2fs.h"
#include "../../include/fs/vfs.h"

#define MAX_VINODE 1024
#define MAX_FILESYSTEM 16
#define VFS_ROOT 0

#define pidx (&vinodes[idx])
#define poidx (&vinodes[oidx])
#define pnidx (&vinodes[nidx])
#define pdot (&vinodes[dot])
#define pddot (&vinodes[ddot])

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
    if (vinodes[i].mode == UNUSED) {
      vinodes[i].mode |= ALLOCED;
      return i;
    }
  return -1;
}

static void vinodes_free(int idx) { vinodes[idx].mode = UNUSED; }

static int first_item_namelen(const char *path) {
  int ret = 0;
  for (; path[ret] && path[ret] != '/';) ret++;
  return ret;
}

static int lookup_cur(char *path, int *pflag, int cur) {
  // cur: vinode_idx(cur_dir)
  if (!strlen(path)) {
    *pflag = 1;
    return cur;
  }

  int k, len = first_item_namelen(path);
  for (k = vinodes[cur].child; k != -1; k = vinodes[k].next) {
    /*
    printf(
        "\nlookup cur: [%d]\n  itemname: %s, path: %s\n  type: "
        "%x, "
        "next: %d, child: %d, dot: %d, ddot: %d\n\n",
        k, vinodes[k].name, vinodes[k].path, vinodes[k].mode, vinodes[k].next,
        vinodes[k].child, vinodes[k].dot, vinodes[k].ddot);
    */
    if (!strncmp(vinodes[k].name, path, len)) {
      printf("match!\n");
      break;
    }
  }

  if (k == -1) {
    *pflag = 0;
    return cur;
  }

  int next = k;
  while (vinodes[next].mode & TYPE_LINK) {
    printf("\nthrough link: %d -> %d\n", next, vinodes[next].next_link);
    next = vinodes[next].next_link;
  }

  char *newpath = path + (len + (path[len] == '/' ? 1 : 0));
  /*
  printf("oldpath: %s, newpath: %s, match_node_name: %s, next: %d\n", path,
         newpath, vinodes[k].name, next);
         */
  return lookup_cur(newpath, pflag, next);
}

static int lookup_root(char *path, int *pflag) {
  return lookup_cur(path + 1, pflag, VFS_ROOT);
}

static int lookup_auto(char *path) {
  int len = strlen(path);
  if (path[len - 1] == '/') path[len - 1] = '\0';

  int flag;
  int idx = (path[0] == '/') ? lookup_root(path, &flag)
                             : lookup_cur(path, &flag, VFS_ROOT);

  if (flag == 1) return idx;
  printf("file no found, but reach: [%d]\n", idx);
  vinode_t buf;
  int kth = 0, oidx = -1, nidx = -1, dot = -1, ddot = -1, ret;
  while ((ret = pidx->fs->readdir(pidx->fs, pidx->rinode_idx, ++kth, &buf))) {
    if ((nidx = vinodes_alloc()) == -1) assert(0);
    // printf("name == %s, oid == %d, kth == %d\n", buf.name, oidx, kth);
    if (!strcmp(buf.name, ".")) {
      assert(oidx == -1);
      assert(pidx->child == -1);
      pidx->child = nidx;

      strcpy(pnidx->name, ".");
      strcpy(pnidx->path, pidx->path);
      pnidx->dot = nidx, pnidx->ddot = -1;
      pnidx->next = -1, pnidx->child = -1;
      pnidx->prev_link = pnidx->next_link = nidx, pnidx->linkcnt = 1;
      pnidx->mode = TYPE_LINK, vinode_add_link(idx, nidx);
      pnidx->rinode_idx = buf.rinode_idx;
      pnidx->fs = pidx->fs;

      dot = nidx;
    } else if (!strcmp(buf.name, "..")) {
      assert(poidx->next == -1);
      poidx->next = nidx;
      poidx->ddot = nidx;

      strcpy(pnidx->name, "..");
      strcpy(pnidx->path, pidx->path);
      pnidx->dot = oidx, pnidx->ddot = idx;
      pnidx->next = -1, pnidx->child = -1;
      pnidx->prev_link = pnidx->next_link = nidx, pnidx->linkcnt = 1;
      pnidx->mode = TYPE_LINK, vinode_add_link(pidx->ddot, nidx);
      pnidx->rinode_idx = buf.rinode_idx;
      pnidx->fs = pidx->fs;

      ddot = nidx;
    } else {
      assert(dot != -1 && ddot != -1);
      assert(poidx->next == -1);
      poidx->next = nidx;

      strcpy(pnidx->name, buf.name);
      strcpy(pnidx->path, pidx->path);
      strcat(pnidx->path, buf.name);
      pnidx->dot = dot, pnidx->ddot = ddot;
      pnidx->next = -1, pnidx->child = -1;
      pnidx->prev_link = pnidx->next_link = nidx, pnidx->linkcnt = 1;
      pnidx->mode = buf.mode;
      pnidx->rinode_idx = buf.rinode_idx;
      pnidx->fs = pidx->fs;
    }

    oidx = nidx;
    /*
        printf("newidx: %d, rinode_idx: %d, name %s, child: %d, next: %d\n",
       nidx, vinodes[nidx].rinode_idx, vinodes[nidx].name, vinodes[nidx].child,
               vinodes[nidx].next);
    */
  }
  return lookup_auto(path);
}

static int filesys_alloc() {
  for (int i = 0; i < MAX_FILESYSTEM; i++)
    if (!strlen(filesys[i].name)) return i;
  return -1;
}

static void filesys_free(int idx) { strcpy(filesys[idx].name, ""); }

static int vfs_init_devfs(const char *name, device_t *dev, size_t size,
                          void (*init)(filesystem_t *, const char *,
                                       device_t *),
                          int (*lookup)(filesystem_t *, char *, int),
                          int (*readdir)(filesystem_t *, int, int,
                                         vinode_t *)) {
  int idx = filesys_alloc();
  strcpy(filesys[idx].name, name);
  filesys[idx].rfs = pmm->alloc(size);
  filesys[idx].dev = dev;
  filesys[idx].init = init;
  filesys[idx].lookup = lookup;
  filesys[idx].readdir = readdir;
  filesys[idx].init(&filesys[idx], name, dev);
  return idx;
}

#define build_dot(CUR, FS)                                          \
  do {                                                              \
    strcpy(pdot->name, ".");                                        \
    strcpy(pdot->path, vinodes[CUR].path);                          \
    pdot->dot = -1, pdot->ddot = ddot;                              \
    pdot->next = ddot, pdot->child = CUR;                           \
    pdot->prev_link = pdot->next_link = dot, pdot->linkcnt = 1;     \
    pdot->mode = TYPE_LINK, vinode_add_link(vinodes[CUR].dot, dot); \
    pdot->fs = FS;                                                  \
  } while (0)

#define build_ddot(PARENT, FS)                                           \
  do {                                                                   \
    strcpy(pddot->name, "..");                                           \
    strcpy(pddot->path, vinodes[PARENT].path);                           \
    pddot->dot = dot, pddot->ddot = -1;                                  \
    pddot->next = -1, pddot->child = PARENT;                             \
    pddot->prev_link = pddot->next_link = ddot, pddot->linkcnt = 1;      \
    pddot->mode = TYPE_LINK, vinode_add_link(vinodes[PARENT].dot, ddot); \
    pddot->fs = FS;                                                      \
  } while (0)

#define build_general_dir(IDX, DOT, DDOT, NAME, FS)        \
  do {                                                     \
    strcpy(vinodes[IDX].name, NAME);                       \
    strcpy(vinodes[IDX].path, vinodes[DOT].path);          \
    strcat(vinodes[IDX].path, NAME);                       \
    vinodes[IDX].dot = DOT, vinodes[IDX].ddot = DDOT;      \
    vinodes[IDX].next = -1, vinodes[IDX].child = -1;       \
    vinodes[IDX].prev_link = vinodes[IDX].next_link = IDX; \
    vinodes[IDX].linkcnt = 1;                              \
    vinodes[IDX].mode = TYPE_DIR;                          \
    vinodes[IDX].fs = FS;                                  \
  } while (0)

int vinodes_build_root() {
  int idx = vinodes_alloc();
  int dot = vinodes_alloc();
  int ddot = vinodes_alloc();
  assert(idx == VFS_ROOT);

  strcpy(pidx->name, "/");
  strcpy(pidx->path, "/");
  pidx->rinode_idx = -1;
  pidx->dot = -1, pidx->ddot = -1;
  pidx->next = -1, pidx->child = dot;
  pidx->prev_link = pidx->next_link = idx;
  pidx->linkcnt = 1;
  pidx->mode = TYPE_DIR | RD_ABLE | WR_ABLE;
  pidx->fs = NULL;

  build_dot(idx, NULL);
  build_ddot(idx, NULL);

  return idx;
}

int vinodes_append_dir(int par, char *name, filesystem_t *fs) {
  // input: vinode_idx("/"), "dev/"
  // modify: "/"
  int nidx = vinodes_alloc(), k = vinodes[par].child, dot = -1, ddot = -1;
  assert(k != -1);

  for (; vinodes[k].next != -1; k = vinodes[k].next) {
    if (!strcmp(vinodes[k].name, ".")) {
      dot = k;
      ddot = vinodes[k].next;
      printf("dot: %d, ddot: %d\n", dot, ddot);
    }
  }
  assert(dot != -1 && ddot != -1);
  vinodes[k].next = nidx;
  build_general_dir(nidx, dot, ddot, name, fs);
  // return new item's idx
  return nidx;
}

int vinodes_create_dir(int idx, int par, filesystem_t *fs) {
  // input: vinode_idx("/dev/"), vinode_idx("/")
  // modify: "/dev/"
  int dot = vinodes_alloc();
  int ddot = vinodes_alloc();

  assert(pidx->child == -1);
  pidx->child = dot;

  build_dot(idx, fs);
  build_ddot(par, fs);
  // return first child
  return dot;
}

int vinodes_mount(int par, char *name, filesystem_t *fs, int rfs_root) {
  // mount /dev/ramdisk0: par = vinode_idx("/dev"), name = "ramdisk0"
  int ret = vinodes_append_dir(par, name, fs);
  vinodes[ret].rinode_idx = rfs_root;
  return ret;
}

typedef struct ext2 ext2_t;
extern void ext2_init(filesystem_t *fs, const char *name, device_t *dev);
extern int ext2_lookup(filesystem_t *fs, char *path, int mode);
extern int ext2_readdir(filesystem_t *fs, int vinode_idx, int kth,
                        vinode_t *buf);

int fuck() {
  lookup_auto("/");
  filesys_free(2);
  vinodes_alloc();
  vinodes_free(0);
  return 0;
}

int vfs_init() {
  int root = vinodes_build_root();
  int dev = vinodes_append_dir(root, "dev/", NULL);

  // printf("fuck");
  vinodes_create_dir(dev, root, NULL);

  int fs_r0 = vfs_init_devfs("ramdisk0", dev_lookup("ramdisk0"), sizeof(ext2_t),
                             ext2_init, ext2_lookup, ext2_readdir);

  vinodes_mount(dev, "ramdisk0/", &filesys[fs_r0], EXT2_ROOT);
  // vinodes_create_dir(r0, dev, &filesys[fs_r0]);

  // return lookup_auto("/dev/ramdisk0/.");
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

  /*
    printf("%s  %d\n", "/", lookup_auto("/"));
    // printf("%s  %d\n", "/dev/", lookup_auto("/dev/"));
    printf("%s  %d\n", "/dev/", lookup_auto("/dev/"));
    printf("%s  %d\n", "/dev/ramdisk0/", lookup_auto("/dev/ramdisk0/"));
    printf("%s  %d\n", "/dev/ramdisk0/hello.cpp/",
           lookup_auto("/dev/ramdisk0/hello.cpp/"));
           */
  lookup_auto("/dev/ramdisk0/directory\0\0");
  lookup_auto("/dev/ramdisk0/directory/\0\0");
  lookup_auto("/dev/ramdisk0/directory/.\0\0");
  return 0;
}

char _path[1024];

int vfs_access(const char *path, int mode) {
  strcpy(_path, path);
  int idx = lookup_auto(_path);
  return vinodes[idx].mode & mode;
}

char *vfs_getpath(const char *path, int mode) {
  strcpy(_path, path);
  int idx = lookup_auto(_path);
  return vinodes[idx].path;
}

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

void vfs_ls(char *dirname) {
  int idx = lookup_auto(dirname);
  if (vinodes[idx].mode & TYPE_DIR && vinodes[idx].child == -1) {
    strcat(dirname, "/.");
    printf("try reach: %s\n", dirname);
    lookup_auto(dirname);
  }

  printf("       index       name                  path\n");
  printf("cur:   %4d        %12s          %s\n\n", idx, vinodes[idx].name,
         vinodes[idx].path);
  for (int k = vinodes[idx].child; k != -1; k = vinodes[k].next) {
    printf("child: %4d        %12s          %s\n", k, vinodes[k].name,
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

int vinode_add_link(int oidx, int nidx) {
  // printf("\n add_link: %d <- %d \n", oidx, nidx);
  int n_link = vinodes[oidx].next_link;
  vinodes[nidx].next_link = n_link;
  vinodes[nidx].prev_link = oidx;
  vinodes[oidx].next_link = nidx;
  vinodes[n_link].prev_link = nidx;
  return 0;
}