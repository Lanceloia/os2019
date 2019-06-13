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

  // printf("FUCK1\n\n\n\n\n\n\n");
  int flag, offset = 1;
  int idx = (path[0] == '/') ? lookup_root(path, &flag, &offset)
                             : lookup_cur(path, &flag, VFS_ROOT, &offset);
  printf("FUCK2\n\n\n\n\n\n\n");
  if (flag == 1) return idx;

  vinode_t buf;
  int kth = 0, oidx = -1, nidx = -1;
  int dot = -1, ddot = -1, ret = -1, next = -1;

  int flen = first_item_len(path + offset);
  printf("%s, %d\n", path + offset, flen);

  while ((ret = pidx->fs->readdir(pidx->fs, pidx->rinode_idx, ++kth, &buf))) {
    if ((nidx = vinodes_alloc()) == -1) assert(0);

    if (!strcmp(buf.name, ".")) {
      assert(oidx == -1);
      assert(pidx->child == -1);
      pidx->child = nidx;

      strcpy(pnidx->name, ".");
      strcpy(pnidx->path, pidx->path);
      pnidx->dot = -1, pnidx->ddot = -1;  // will be cover
      pnidx->next = -1, pnidx->child = idx;
      pnidx->prev_link = pnidx->next_link = nidx, pnidx->linkcnt = 1;
      pnidx->mode = TYPE_LINK, vinode_add_link(idx, nidx, 4);

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
      pnidx->mode = TYPE_LINK,
      vinode_add_link(vinodes[pidx->dot].child, nidx, 3);

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

    pnidx->rinode_idx = buf.rinode_idx;
    pnidx->fs = pidx->fs;
    oidx = nidx;

    if (item_match(buf.name, path + offset, flen)) {
      printf("read: %s, %d, %d\n\n", path + offset, flen, next);
      assert(next == -1);
      next = nidx;
    } else {
      printf("fuck? %s\n", vinodes[nidx].name);
      /*
      printf("offset: %d, path: %s, path + offset: %s\n", offset, path,
             path + offset);
             */
    }
  }
  // assert(0);
  assert(next != -1);
  printf("fuck: %d", next);
  if (next == -1) return 0;

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

#define build_dot(CUR, FS)                                      \
  do {                                                          \
    strcpy(pdot->name, ".");                                    \
    strcpy(pdot->path, vinodes[CUR].path);                      \
    pdot->dot = -1, pdot->ddot = ddot;                          \
    pdot->next = ddot, pdot->child = CUR;                       \
    pdot->prev_link = pdot->next_link = dot, pdot->linkcnt = 1; \
    pdot->mode = TYPE_LINK, vinode_add_link(CUR, dot, 1);       \
    pdot->fs = FS;                                              \
  } while (0)

#define build_ddot(PARENT, FS)                                      \
  do {                                                              \
    strcpy(pddot->name, "..");                                      \
    strcpy(pddot->path, vinodes[PARENT].path);                      \
    pddot->dot = dot, pddot->ddot = -1;                             \
    pddot->next = -1, pddot->child = PARENT;                        \
    pddot->prev_link = pddot->next_link = ddot, pddot->linkcnt = 1; \
    pddot->mode = TYPE_LINK, vinode_add_link(PARENT, ddot, 2);      \
    pddot->fs = FS;                                                 \
  } while (0)

#define build_general_null_dir(IDX, DOT, DDOT, NAME, FS)   \
  do {                                                     \
    strcpy(vinodes[IDX].name, NAME);                       \
    strcpy(vinodes[IDX].path, vinodes[DOT].path);          \
    strcat(vinodes[IDX].path, NAME);                       \
    strcat(vinodes[IDX].path, "/");                        \
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
  // printf("\n\n\n\n\ndot.child: %d\n", vinodes[dot].child);
  build_general_null_dir(nidx, dot, ddot, name, fs);
  // return new dir's idx
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
extern int ext2_lookup(filesystem_t *fs, char *path, int mode);
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
  int root = vinodes_build_root();
  int dev = vinodes_append_dir(root, "dev", NULL);

  // printf("fuck");
  vinodes_create_dir(dev, root, NULL);

  int fs_r0 = vfs_init_devfs("ramdisk0", dev_lookup("ramdisk0"), sizeof(ext2_t),
                             ext2_init, ext2_lookup, ext2_readdir);

  vinodes_mount(dev, "ramdisk0", &filesys[fs_r0], EXT2_ROOT);

  // lookup_auto("/dev/ramdisk0/directory\0\0");
  // lookup_auto("/dev/ramdisk0/directory/\0\0");
  lookup_auto("/dev/asaddasd\0\0");

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

int vfs_open(const char *path, int mode) {
  if (!vfs_access(path, mode)) return -1;

  int idx = lookup_auto(_path);
  return vinode_open(idx, mode);
}

ssize_t vfs_read(int fd, char *buf, size_t nbyte) {
  assert(nbyte <= 1024);
  extern ssize_t ext2_read(ext2_t * ext2, int rinode_idx, char *buf,
                           uint32_t len);
  return ext2_read(vinodes[files[fd].vinode_idx].fs->rfs,
                   vinodes[files[fd].vinode_idx].rinode_idx, buf, nbyte);
}

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

  printf("<     index       name                  path       >\n");
  printf("      %4d        %12s          %s\n", idx, vinodes[idx].name,
         vinodes[idx].path);
  for (int k = vinodes[idx].child; k != -1; k = vinodes[k].next) {
    printf("      %4d        %12s          %s\n", k, vinodes[k].name,
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

int vinode_add_link(int oidx, int nidx, int flag) {
  // printf("\n add_link: %d <- %d : %d\n", oidx, nidx, flag);
  int n_link = vinodes[oidx].next_link;
  vinodes[nidx].next_link = n_link;
  vinodes[nidx].prev_link = oidx;
  vinodes[oidx].next_link = nidx;
  vinodes[n_link].prev_link = nidx;
  return 0;
}