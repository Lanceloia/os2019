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
file_t files[MAX_FILE];

typedef struct device device_t;
extern device_t *dev_lookup(const char *name);

// helper
static int vinodes_alloc();
static void vinodes_free(int idx);
static int lookup_cur(char *path, int *flag, int cur, int *poffset);
static int lookup_root(char *path, int *flag, int *poffset);
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

static int first_item_len(const char *path) {
  int ret = 0;
  for (; path[ret] && path[ret] != '/';) ret++;
  return ret;
}

static int item_match(const char *s1, const char *s2, int len) {
  // printf("P: %s\nT: %s\nlen: %d\n\n", s1, s2, len);
  if (strncmp(s1, s2, len)) return 0;
  return s1[len] == '\0';
}

static int lookup_cur(char *path, int *pflag, int cur, int *poffset) {
  // cur: vinode_idx(cur_dir)
  if (!strlen(path)) {
    *pflag = 1;
    return cur;
  }
  // printf("path: %s\n", path);
  int k, len = first_item_len(path);
  for (k = vinodes[cur].child; k != -1; k = vinodes[k].next) {
    if (item_match(vinodes[k].name, path, len)) {
      // printf("name: %s\n", vinodes[k].name);
      break;
    }
  }

  if (k == -1) {
    // assert(0);
    *pflag = 0;
    return cur;
  }

  int next = k;
  while (vinodes[next].mode & TYPE_LINK) next = vinodes[next].next_link;

  char *newpath = path + (len + (path[len] == '/' ? 1 : 0));
  *poffset += len + (path[len] == '/' ? 1 : 0);
  return lookup_cur(newpath, pflag, next, poffset);
}

static int lookup_root(char *path, int *pflag, int *poffset) {
  return lookup_cur(path + 1, pflag, VFS_ROOT, poffset);
}

static int lookup_auto(char *path) {
  int len = strlen(path);
  if (path[len - 1] == '/') path[len - 1] = '\0';

  int flag, offset = 1;
  int idx = (path[0] == '/') ? lookup_root(path, &flag, &offset)
                             : lookup_cur(path, &flag, VFS_ROOT, &offset);
  if (flag == 1) return idx;

  vinode_t buf;
  int kth = 0, oidx = -1, nidx = -1;
  int dot = -1, ddot = -1, ret = -1, next = -1;

  int flen = first_item_len(path + offset);
  // printf("%s, %d\n", path + offset, flen);

  if (pidx->fs == NULL) return -1;

  if (pidx->child != -1) return -1;

  while ((ret = pidx->fs->readdir(pidx->fs, pidx->ridx, ++kth, &buf))) {
    if ((nidx = vinodes_alloc()) == -1) assert(0);

    if (!strcmp(buf.name, ".")) {
      assert(oidx == -1);

      pidx->child = nidx;

      strcpy(pnidx->name, ".");
      strcpy(pnidx->path, pidx->path);
      pnidx->dot = -1, pnidx->ddot = -1;  // will be cover
      pnidx->next = -1, pnidx->child = idx;
      pnidx->prev_link = pnidx->next_link = nidx, pnidx->linkcnt = 1;
      pnidx->mode = TYPE_LINK, add_link(idx, nidx);

      dot = nidx;
    } else if (!strcmp(buf.name, "..")) {
      assert(poidx->next == -1);
      poidx->next = nidx;
      poidx->ddot = nidx;
      strcpy(pnidx->name, "..");
      strcpy(pnidx->path, vinodes[vinodes[pidx->dot].child].path);
      pnidx->dot = oidx, pnidx->ddot = -1;
      pnidx->next = -1, pnidx->child = vinodes[pidx->ddot].child;
      pnidx->prev_link = pnidx->next_link = nidx, pnidx->linkcnt = 1;
      pnidx->mode = TYPE_LINK, add_link(vinodes[pidx->dot].child, nidx);

      ddot = nidx;
    } else {
      assert(dot != -1 && ddot != -1);
      assert(poidx->next == -1);
      poidx->next = nidx;
      strcpy(pnidx->name, buf.name);
      strcpy(pnidx->path, pidx->path);
      strcat(pnidx->path, buf.name);
      if (buf.mode & TYPE_DIR) strcat(pnidx->path, "/");
      pnidx->dot = dot, pnidx->ddot = ddot;
      pnidx->next = -1, pnidx->child = -1;
      pnidx->prev_link = pnidx->next_link = nidx, pnidx->linkcnt = 1;
      pnidx->mode = buf.mode;
    }

    pnidx->ridx = buf.ridx;
    pnidx->fs_type = pidx->fs_type;
    pnidx->fs = pidx->fs;
    oidx = nidx;

    if (item_match(buf.name, path + offset, flen)) {
      // printf("read: %s, %d, %d\n\n", path + offset, flen, next);
      assert(next == -1);
      next = nidx;
    }
  }

  if (next == -1) {
    printf("read directory, but file is not exists!\n");
    return -1;
  }

  int noffset = 1;
  idx = (path[0] == '/') ? lookup_root(path, &flag, &noffset)
                         : lookup_cur(path, &flag, VFS_ROOT, &noffset);
  assert(noffset > offset);
  return (noffset == offset) ? -1 : lookup_auto(path);
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
                          int (*readdir)(filesystem_t *, int, int,
                                         vinode_t *)) {
  int idx = filesys_alloc();
  strcpy(filesys[idx].name, name);
  filesys[idx].rfs = pmm->alloc(size);
  filesys[idx].dev = dev;
  filesys[idx].init = init;
  filesys[idx].readdir = readdir;
  filesys[idx].init(&filesys[idx], name, dev);
  return idx;
}

#define build_dot(CUR, FSTYPE, FS)                              \
  do {                                                          \
    strcpy(pdot->name, ".");                                    \
    strcpy(pdot->path, vinodes[CUR].path);                      \
    pdot->dot = -1, pdot->ddot = ddot;                          \
    pdot->next = ddot, pdot->child = CUR;                       \
    pdot->prev_link = pdot->next_link = dot, pdot->linkcnt = 1; \
    pdot->mode = TYPE_LINK, add_link(CUR, dot);                 \
    pdot->fs_type = FSTYPE;                                     \
    pdot->fs = FS;                                              \
  } while (0)

#define build_ddot(PARENT, FSTYPE, FS)                              \
  do {                                                              \
    strcpy(pddot->name, "..");                                      \
    strcpy(pddot->path, vinodes[PARENT].path);                      \
    pddot->dot = dot, pddot->ddot = -1;                             \
    pddot->next = -1, pddot->child = PARENT;                        \
    pddot->prev_link = pddot->next_link = ddot, pddot->linkcnt = 1; \
    pddot->mode = TYPE_LINK, add_link(PARENT, ddot);                \
    pddot->fs_type = FSTYPE;                                        \
    pddot->fs = FS;                                                 \
  } while (0)

#define build_general_null_dir(IDX, DOT, DDOT, NAME, FSTYPE, FS) \
  do {                                                           \
    strcpy(vinodes[IDX].name, NAME);                             \
    strcpy(vinodes[IDX].path, vinodes[DOT].path);                \
    strcat(vinodes[IDX].path, NAME);                             \
    strcat(vinodes[IDX].path, "/");                              \
    vinodes[IDX].dot = DOT, vinodes[IDX].ddot = DDOT;            \
    vinodes[IDX].next = -1, vinodes[IDX].child = -1;             \
    vinodes[IDX].prev_link = vinodes[IDX].next_link = IDX;       \
    vinodes[IDX].linkcnt = 1;                                    \
    vinodes[IDX].mode = TYPE_DIR;                                \
    vinodes[IDX].fs_type = FSTYPE;                               \
    vinodes[IDX].fs = FS;                                        \
  } while (0)

#define delete_vinode(IDX) \
  do {                     \
    vinodes_free(IDX);     \
    remove_link(IDX);      \
  } while (0)

static int build_root() {
  int idx = vinodes_alloc();
  int dot = vinodes_alloc();
  int ddot = vinodes_alloc();
  assert(idx == VFS_ROOT);

  strcpy(pidx->name, "/");
  strcpy(pidx->path, "/");
  pidx->ridx = -1;
  pidx->dot = -1, pidx->ddot = -1;
  pidx->next = -1, pidx->child = dot;
  pidx->prev_link = pidx->next_link = idx;
  pidx->linkcnt = 1;
  pidx->mode = TYPE_DIR | RD_ABLE | WR_ABLE;
  pidx->fs_type = VFS;
  pidx->fs = NULL;

  build_dot(idx, VFS, NULL);
  build_ddot(idx, VFS, NULL);

  return idx;
}

static int append_dir(int par, char *name, int fy_type, filesystem_t *fs) {
  int nidx = vinodes_alloc(), k = vinodes[par].child, dot = -1, ddot = -1;
  assert(k != -1);

  for (; vinodes[k].next != -1; k = vinodes[k].next) {
    if (!strcmp(vinodes[k].name, ".")) {
      dot = k;
      ddot = vinodes[k].next;
      // printf("dot: %d, ddot: %d\n", dot, ddot);
    }
  }
  assert(dot != -1 && ddot != -1);
  vinodes[k].next = nidx;
  build_general_null_dir(nidx, dot, ddot, name, fy_type, fs);
  return nidx;
}

static int prepare_dir(int idx, int par, int fs_type, filesystem_t *fs) {
  int dot = vinodes_alloc();
  int ddot = vinodes_alloc();

  assert(pidx->child == -1);
  pidx->child = dot;

  build_dot(idx, fs_type, fs);
  build_ddot(par, fs_type, fs);
  return dot;
}

static int remove_dir(int idx, int par) {
  // printf("%d %d fuck! \n", idx, par);
  int pre = vinodes[par].child;
  for (; vinodes[pre].next != idx;) pre = vinodes[pre].next;
  assert(vinodes[pre].next == idx);
  vinodes[pre].next = pidx->next;
  // printf("d: %d, dd: %d\n", pidx->dot, pidx->ddot);
  for (int k = pidx->child; k != -1; k = vinodes[k].next) {
    delete_vinode(k);
  }
  // delete_vinode(pidx->ddot);
  delete_vinode(idx);
  return 0;
}

int vinodes_mount(int par, char *name, int fs_type, filesystem_t *fs) {
  // mount /dev/ramdisk0: par = vinode_idx("/dev"), name = "ramdisk0"
  int ret = append_dir(par, name, fs_type, fs);
  switch (fs_type) {
    case VFS:
      vinodes[ret].ridx = VFS_ROOT;
      break;
    case EXT2FS:
      vinodes[ret].ridx = EXT2_ROOT;
      break;
    default:
      assert(0);
      break;
  }
  return ret;
}

//
static int files_alloc() {
  for (int i = 0; i < MAX_FILE; i++)
    if (files[i].refcnt == 0) return i;
  return -1;
}

static void files_free(int fd) { files[fd].refcnt = 0; }

int vinode_open(int inode_idx, int mode) {
  int fd = files_alloc();
  if (fd == -1) return -1;
  files[fd].vinode_idx = inode_idx;
  files[fd].offset = 0;
  files[fd].refcnt++;
  return fd;
}

typedef struct ext2 ext2_t;
extern void ext2_init(filesystem_t *fs, const char *name, device_t *dev);
extern int ext2_readdir(filesystem_t *fs, int vinode_idx, int kth,
                        vinode_t *buf);

int fuck() {
  lookup_auto("/");
  filesys_free(2);
  vinodes_alloc();
  vinodes_free(0);
  files_free(9);
  return 0;
}

int vfs_init() {
  int root = build_root();
  int dev = append_dir(root, "dev", VFS, NULL);

  // printf("fuck");
  prepare_dir(dev, root, VFS, NULL);

  int fs_r0 = vfs_init_devfs("ramdisk0", dev_lookup("ramdisk0"), sizeof(ext2_t),
                             ext2_init, ext2_readdir);
  int fs_r1 = vfs_init_devfs("ramdisk1", dev_lookup("ramdisk1"), sizeof(ext2_t),
                             ext2_init, ext2_readdir);
  vinodes_mount(dev, "ramdisk0", EXT2FS, &filesys[fs_r0]);
  vinodes_mount(dev, "ramdisk1", EXT2FS, &filesys[fs_r1]);

  // lookup_auto("/dev/ramdisk0/directory\0\0");
  // lookup_auto("/dev/ramdisk0/directory/\0\0");
  // lookup_auto("/dev/asaddasd\0\0");

  return 0;
}

char tmppath[1024];

int vfs_access(const char *path, int mode) {
  strcpy(tmppath, path);
  int idx = lookup_auto(tmppath);
  return vinodes[idx].mode & mode;
}

char *vfs_getpath(const char *path) {
  strcpy(tmppath, path);
  int idx = lookup_auto(tmppath);
  return vinodes[idx].path;
}

int vfs_help_getpathlen(const char *path) {
  int len = strlen(path);
  for (int i = len - 1; i >= 0; i--)
    if (path[i] == '/') return i;
  return 0;
}

int vfs_mount(const char *path, filesystem_t *fs) { return 0; }

int vfs_unmount(const char *path) { return 0; }

extern int ext2_create(ext2_t *, int, char *, int);
extern int ext2_remove(ext2_t *, int, char *, int);

int vfs_mkdir(const char *path) {
  int len = strlen(path);
  int offset = vfs_help_getpathlen(path);

  if (len == offset) {
    printf("Incorrect pathname! \n");
    return 1;
  }

  strcpy(tmppath, path);
  tmppath[offset] = '\0';

  int idx = lookup_auto(tmppath);
  int ridx = -1, nidx = -1;
  switch (pidx->fs_type) {
    case EXT2FS:
      ridx = ext2_create(pidx->fs->rfs, pidx->ridx, tmppath + offset + 1,
                         TYPE_DIR);
      nidx = append_dir(idx, tmppath + offset + 1, pidx->fs_type, pidx->fs);
      prepare_dir(nidx, idx, pidx->fs_type, pidx->fs);
      pnidx->ridx = ridx;
      break;

    default:
      printf("Cannot mkdir here! \n");
      break;
  }
  return 0;
}

int vfs_rmdir(const char *path) {
  int len = strlen(path);
  int offset = vfs_help_getpathlen(path);

  if (len == offset) {
    printf("Incorrect pathname! \n");
    return 1;
  }

  strcpy(tmppath, path);
  int nidx = lookup_auto(tmppath);
  tmppath[offset] = '\0';
  int idx = lookup_auto(tmppath);

  // printf("fuck! \n");
  // assert(idx == pnidx->ddot)

  int ret = 1;

  switch (pidx->fs_type) {
    case EXT2FS:
      ret = ext2_remove(pidx->fs->rfs, idx, tmppath + offset + 1, TYPE_DIR);
      if (!ret) remove_dir(nidx, idx);
      break;

    default:
      printf("Cannot rmdir here! \n");
      break;
  }
  return ret;
}

int vfs_link(const char *oldpath, const char *newpath) { return 0; }

int vfs_unlink(const char *path) { return 0; }

int vfs_open(const char *path, int mode) {
  if (!vfs_access(path, mode)) return -1;

  int idx = lookup_auto(tmppath);
  return vinode_open(idx, mode);
}

#define pfd (&vinodes[files[fd].vinode_idx])

extern ssize_t ext2_read(ext2_t *, int, uint64_t, char *, uint32_t);
extern ssize_t ext2_write(ext2_t *, int, uint64_t, char *, uint32_t);

ssize_t vfs_read(int fd, char *buf, size_t nbyte) {
  if (fd < 0) return 0;
  assert(nbyte <= 1024);

  int ret = 0;
  switch (pfd->fs_type) {
    case EXT2FS:
      ret = ext2_read(pfd->fs->rfs, pfd->ridx, files[fd].offset, buf, nbyte);
      files[fd].offset += ret;
      break;
    default:
      printf("Cannot read here! \n");
      break;
  }
  return ret;
}

ssize_t vfs_write(int fd, char *buf, size_t nbyte) {
  if (fd < 0) return 0;
  assert(nbyte <= 1024);
  int ret = 0;
  switch (pfd->fs_type) {
    case EXT2FS:
      ret = ext2_write(pfd->fs->rfs, pfd->ridx, files[fd].offset, buf, nbyte);
      files[fd].offset += ret;
      break;
    default:
      printf("Cannot write here! \n");
      break;
  }
  return ret;
}

off_t vfs_lseek(int fd, off_t offset, int whence) { return 0; }

int vfs_close(int fd) { return 0; }

void vfs_ls(char *dirname) {
  printf("\n");
  int idx = lookup_auto(dirname);
  if (idx == -1) return;
  printf("-     index       name                  path        \n");
  printf(">>   %4d        %12s          %s\n", idx, vinodes[idx].name,
         vinodes[idx].path);
  for (int k = vinodes[idx].child; k != -1; k = vinodes[k].next) {
    printf("-    %4d        %12s          %s\n", k, vinodes[k].name,
           vinodes[k].path);
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
};

#ifdef _LANCELOIA_DEBUG_
#undef _LANCELOIA_DEBUG_
#endif

void add_link(int oidx, int nidx) {
  int n_link = poidx->next_link;
  pnidx->next_link = n_link;
  pnidx->prev_link = oidx;
  poidx->next_link = nidx;
  vinodes[n_link].prev_link = nidx;
}

void remove_link(int idx) {
  int p_link = pidx->prev_link;
  int n_link = pidx->next_link;

  vinodes[p_link].next_link = pidx->next_link;
  vinodes[n_link].prev_link = pidx->prev_link;
}