#include <klib.h>

#include "../../../include/fs/ext2fs.h"
#include "../../../include/fs/vfs.h"

char readbuf[128], writebuf[128];
char bigbuf[2048] = {};

char abs_path[256], abs_path2[256];

static void build_abs_path(char *dirname, char *pwd) {
  if (dirname[0] != '/') {
    strcpy(abs_path, pwd);
    strcat(abs_path, dirname);
  } else {
    strcpy(abs_path, dirname);
  }
}

/* shell command */

static void pwd_do(device_t *tty, char *unused, char *pwd) {
  sprintf(bigbuf, "%s\n", pwd);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void echo_do(device_t *tty, char *str, char *pwd) {
  sprintf(bigbuf, "%s\n", str);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
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
  printf("path: %s\n", abs_path);
  while (1) {
    int nread = tty->ops->read(tty, 0, readbuf, sizeof(readbuf));
    // printf("fuck?\n");
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
  switch (vfs_create(abs_path)) {
    case 0:
      printf("Success! \n");
      break;
    case 1:
      printf("Failed! \n");
      break;

    default:
      break;
  }

  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void rmdir_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  if (!vfs_access(abs_path, TYPE_DIR)) printf("Dir is not exists! \n");
  switch (vfs_remove(abs_path)) {
    case 0:
      printf("Success! \n");
      break;
    case 1:
      printf("The dir is no empty! \n");
      break;
    case 2:
      printf("That is not a dir! \n");
      break;
    default:
      assert(0);
      break;
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void link_do(device_t *tty, char *argv, char *pwd) {
  int offset = 0;
  for (; argv[offset] && argv[offset] != ' ';) offset++;
  argv[offset] = '\0';
  build_abs_path(argv, pwd);
  strcpy(abs_path2, abs_path);
  build_abs_path(argv + offset + 1, pwd);
  vfs_link(abs_path, abs_path2);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

struct shellinfo {
  char *name;
  char *script;
  void (*func)(device_t *tty, char *argv, char *pwd);
  int offset;
} INFO[] = {
    {"link ", "  link [oldpath] [newpath]   (link path)", link_do, 5},
    {"rmdir ", "  rmdir [dirname]   (remove directory)", rmdir_do, 6},
    {"mkdir ", "  mkdir [dirname]   (make directory)", mkdir_do, 6},
    {"cat > ", "  cat > [filename]  (write file, end with '~')", catto_do, 6},
    {"cat ", "  cat [filename]    (read file)", cat_do, 4},
    {"cd ", "  cd [dirname]      (change directory)", cd_do, 3},
    {"ls ", "  ls [dirname]      (list directory's items)", ls_do, 3},
    {"echo ", "  echo [expr]       (print expreesion)", echo_do, 5},
    {"pwd ", "  pwd               (print work directory)", pwd_do, 4}};

static void default_do(device_t *tty) {
  int offset = 0;
  offset += sprintf(bigbuf + offset, "Unexpected command\n");
  offset += sprintf(bigbuf + offset, "\n    help    \n");
  for (int i = 0; i < sizeof(INFO) / sizeof(struct shellinfo); i++)
    offset += sprintf(bigbuf + offset, "%s\n", INFO[i].script);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

char pwd[1024] = "/";

void shell_task(void *name) {
  device_t *tty = dev_lookup(name);
  // vfsdirs_alloc(name, dev_dir, TTY, total_dev_cnt++);

  while (1) {
    sprintf(writebuf, "(%s) $ ", name);
    tty->ops->write(tty, 0, writebuf, strlen(writebuf));
    int nread = tty->ops->read(tty, 0, readbuf, sizeof(readbuf));
    readbuf[nread - 1] = '\0';

    if (!strcmp(readbuf, "ls")) strcpy(readbuf, "ls .");
    if (!strcmp(readbuf, "cd")) strcpy(readbuf, "cd .");
    if (!strcmp(readbuf, "pwd")) strcpy(readbuf, "pwd ");

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