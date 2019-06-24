#include <klib.h>

#include "../../../include/fs/ext2fs.h"
#include "../../../include/fs/vfs.h"

char readbuf[128], writebuf[128];
char bigbuf[2048] = {};

/* shell command */

/*
static void pwd_do(device_t *tty, char *pwd) {
  sprintf(bigbuf, "%s\n", pwd);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}
*/

static void echo_do(device_t *tty, char *str) {
  sprintf(bigbuf, "%s\n", str);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

/*
extern void vfs_cd(char *dirname, char *pwd, char *out);
extern void ext2_cd(ext2_t *ext2, char *dirname, char *pwd, char *out);
extern void procfs_cd(char *dirname, char *pwd, char *out);
static void cd_do(device_t *tty, char *dirname, char *pwd) {
  int type = vfs_identify_fs(pwd);
  switch (type & ~INTERFACE) {
    case VFS:  // vfs
      vfs_cd(dirname, pwd, bigbuf);
      break;
    case EXT2:  // ext2
      if ((type & INTERFACE) &&
          (!strcmp(dirname, ".") || !strcmp(dirname, "..")))
        vfs_cd(dirname, pwd, bigbuf);
      else
        ext2_cd(vfs_get_real_fs(pwd), dirname, pwd, bigbuf);
      break;
    case PROCFS:
      if ((type & INTERFACE) &&
          (!strcmp(dirname, ".") || !strcmp(dirname, "..")))
        vfs_cd(dirname, pwd, bigbuf);
      else
        procfs_cd(dirname, pwd, bigbuf);
      break;
    default:
      sprintf(bigbuf, "can't cd here.\n", bigbuf);
      break;
  };
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}
*/

/*
extern void vfs_ls(char *dirname, char *pwd, char *out);
extern void ext2_ls(ext2_t *ext2, char *dirname, char *out);
extern void procfs_ls(char *dirname, char *out);
static void ls_do(device_t *tty, char *dirname, char *pwd) {
  int type = vfs_identify_fs(pwd);
  switch (type & ~INTERFACE) {
    case VFS:  // vfs
      vfs_ls(dirname, pwd, bigbuf);
      break;
    case EXT2:  // ext2
      ext2_ls(vfs_get_real_fs(pwd), dirname, bigbuf);
      break;
    case PROCFS:  // procfs
      procfs_ls(dirname, bigbuf);
      break;
    default:
      sprintf(bigbuf, "can't ls here.\n", bigbuf);
      break;
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}
*/

char abs_path[256];

static void build_abs_path(char *dirname, char *pwd) {
  if (dirname[0] != '/') {
    strcpy(abs_path, pwd);
    strcat(abs_path, dirname);
  } else {
    strcpy(abs_path, dirname);
  }
}

static void ls_do(device_t *tty, char *dirname, char *pwd) {
  extern void vfs_ls(char *path);
  build_abs_path(dirname, pwd);
  vfs_ls(abs_path);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void cd_do(device_t *tty, char *dirname, char *pwd) {
  extern char *vfs_getpath(const char *dirname);
  build_abs_path(dirname, pwd);
  if (vfs_access(abs_path, TYPE_DIR)) {
    strcpy(pwd, vfs_getpath(abs_path));
  }
  printf("Current: %s\n", pwd);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void cat_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  int fd = vfs_open(abs_path, TYPE_FILE | RD_ABLE);
  while (vfs_read(fd, bigbuf, 1024))
    tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
  bigbuf[0] = '\n', bigbuf[1] = '\0';
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void catto_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  int fd = vfs_open(abs_path, TYPE_FILE | WR_ABLE);
  while (1) {
    int nread = tty->ops->read(tty, 0, readbuf, sizeof(readbuf));
    if (readbuf[nread - 2] == '~') {
      vfs_write(fd, readbuf, nread - 2);
      break;
    } else {
      vfs_write(fd, readbuf, nread);
    }
  }
}

static void mkdir_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  if (vfs_access(abs_path, TYPE_DIR)) {
    printf("Dir is exists! \n");
    return;
  }
  if (vfs_create(abs_path)) {
    printf("Cannot mkdir here! \n");
    return;
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void rmdir_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  if (!vfs_access(abs_path, TYPE_DIR)) printf("Dir is not exists! \n");
  if (vfs_remove(abs_path)) printf("Cannot rmdir here! \n");
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

struct shellinfo {
  char *name;
  char *script;
  void (*func)(device_t *tty, char *argv, char *pwd);
  int offset;
} INFO[] = {
    {"echo ", "  echo [expr]     (print expreesion)", ls_do, 5},
    {"ls ", "  ls [dirname]     (list directory's items)", ls_do, 3},
    {"cd ", "  cd [dirname]     (change directory)", cd_do, 3},
    {"cat ", "  cat [filename]   (read file)", cat_do, 4},
    {"cat > ", "  cat > [filename]  (write file, end: '~')", catto_do, 6},
    {"mkdir ", "  mkdir [dirname]  (make directory)", mkdir_do, 6},
    {"rmdir ", "  rmdir [dirname]  (remove directory)", rmdir_do, 6},
};

static void default_do(device_t *tty) {
  int offset = 0;
  offset += sprintf(bigbuf + offset, "Unexpected command\n");
  for (int i = 0; i < sizeof(INFO) / sizeof(struct shellinfo); i++)
    offset += sprintf(bigbuf + offset, "%s\n", INFO[i].script);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

void shell_task(void *name) {
  device_t *tty = dev_lookup(name);
  // vfsdirs_alloc(name, dev_dir, TTY, total_dev_cnt++);
  char pwd[256] = "/";
  while (1) {
    sprintf(writebuf, "(%s) $ ", name);
    tty->ops->write(tty, 0, writebuf, strlen(writebuf));
    int nread = tty->ops->read(tty, 0, readbuf, sizeof(readbuf));
    readbuf[nread - 1] = '\0';

    if (!strcmp(readbuf, "ls")) strcpy(readbuf, "ls .");
    if (!strcmp(readbuf, "cd")) strcpy(readbuf, "cd .");

    /*
    if (!strcmp(readbuf, "pwd"))
      pwd_do(tty, pwd);
    */
    for (int i = 0; i < sizeof(INFO) / sizeof(struct shellinfo); i++) {
      if (!strncmp(readbuf, INFO[i].name, INFO[i].offset)) {
        (*INFO[i].func)(tty, readbuf + INFO[i].offset, pwd);
        goto End;
      }
    }
    default_do(tty);

  End:
    sprintf(bigbuf, "");
  }
}