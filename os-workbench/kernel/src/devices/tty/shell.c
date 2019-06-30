#include <klib.h>

#include "../../../include/fs/ext2fs.h"
#include "../../../include/fs/vfs.h"

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
  extern void vfs_ls(char *path, char *buf);
  build_abs_path(dirname, pwd);
  if (abs_path[strlen(abs_path) - 1] == '/')
    strcat(abs_path, ".");
  else
    strcat(abs_path, "/.");
  vfs_ls(abs_path, bigbuf);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void cd_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  if (vfs_access(abs_path, TYPE_DIR)) {
    sprintf(bigbuf, "That is not a dir! \n");
  } else {
    if (abs_path[strlen(abs_path) - 1] == '/')
      strcat(abs_path, ".");
    else
      strcat(abs_path, "/.");
    if (!vfs_access(abs_path, TYPE_DIR)) {
      extern char *vfs_getpath(const char *dirname);
      strcpy(pwd, vfs_getpath(abs_path));
    }
    sprintf(bigbuf, "Current: %s\n", pwd);
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void cat_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  int fd = vfs_open(abs_path, TYPE_FILE | RD_ABLE);
  if (fd == -1) {
    sprintf(bigbuf, "That is not a readable file! \n");
    tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
  } else {
    while (vfs_read(fd, bigbuf, 1024))
      tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
  }
}

static void catto_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  int fd = vfs_open(abs_path, TYPE_FILE | WR_ABLE);
  if (fd == -1) {
    sprintf(bigbuf, "That is not a writeable file! \n");
    tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
  } else {
    while (1) {
      int nread = tty->ops->read(tty, 0, bigbuf, sizeof(bigbuf));
      // printf("fuck?\n");
      if (bigbuf[nread - 2] == '~') {
        vfs_write(fd, bigbuf, nread - 2);
        break;
      } else {
        vfs_write(fd, bigbuf, nread);
      }
    }
  }
}

static void mkdir_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  if (!vfs_access(abs_path, TYPE_DIR)) {
    sprintf(bigbuf, "Dir is exists! \n");
    return;
  }
  switch (vfs_create(abs_path)) {
    case 0:
      sprintf(bigbuf, "Success! \n");
      break;
    case 1:
      sprintf(bigbuf, "Incorrect pathname! \n");
      break;
    case 2:
      sprintf(bigbuf, "Uncapable filesystem! \n");
      break;

    default:
      break;
  }

  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void rmdir_do(device_t *tty, char *dirname, char *pwd) {
  build_abs_path(dirname, pwd);
  if (vfs_access(abs_path, TYPE_DIR)) sprintf(bigbuf, "Dir is not exists! \n");
  switch (vfs_remove(abs_path)) {
    case 0:
      sprintf(bigbuf, "Success! \n");
      break;
    case 1:
      sprintf(bigbuf, "Incorrect pathname! \n");
      break;
    case 2:
      sprintf(bigbuf, "Uncapable filesystem! \n");
      break;
    case 3:
      sprintf(bigbuf, "Cannot remove! \n");
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
  switch (vfs_link(abs_path2, abs_path)) {
    case 0:
      sprintf(bigbuf, "Success! \n");
      break;
    case 1:
      sprintf(bigbuf, "Oldpath is not exists! \n");
      break;
    case 2:
      sprintf(bigbuf, "Newpath is exists! \n");
      break;
    default:
      assert(0);
      break;
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void unlink_do(device_t *tty, char *path, char *pwd) {
  build_abs_path(path, pwd);
  switch (vfs_unlink(abs_path)) {
    case 0:
      sprintf(bigbuf, "Success! \n");
      break;
    case 1:
      sprintf(bigbuf, "Failed! \n");
      break;
    default:
      assert(0);
      break;
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void mount_do(device_t *tty, char *argv, char *pwd) {
  int offset = 0;
  for (; argv[offset] && argv[offset] != ' ';) offset++;
  argv[offset] = '\0';
  build_abs_path(argv, pwd);
  strcpy(abs_path2, abs_path);
  build_abs_path(argv + offset + 1, pwd);
  switch (vfs_mount(abs_path2, abs_path)) {
    case 0:
      sprintf(bigbuf, "Success! \n");
      break;
    case 1:
      sprintf(bigbuf, "Uncapable filesystem! \n");
      break;
    case 2:
      sprintf(bigbuf, "Dir already exists! \n");
      break;
    default:
      assert(0);
      break;
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void unmount_do(device_t *tty, char *argv, char *pwd) {
  int offset = 0;
  build_abs_path(argv, pwd);
  switch (vfs_unmount(abs_path)) {
    case 0:
      sprintf(bigbuf, "Success! \n");
      break;
    case 1:
      sprintf(bigbuf, "Incorrect pathname! \n");
      break;
    default:
      assert(0);
      break;
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

struct shellinfo {
  char *name;
  char *script;
  void (*func)(device_t *tty, char *argv, char *pwd);
  int offset;
} INFO[] = {
    {"unlink ", "  unlink [path]            (unlink [path])", unlink_do, 7},
    {"link ", "  link [oldpath] [newpath] (link two [path])", link_do, 5},
    {"mount ", "  mount [fs] [path] (mount [fs] to [path])", mount_do, 6},
    {"rmdir ", "  rmdir [dirname]   (remove [dirname])", rmdir_do, 6},
    {"mkdir ", "  mkdir [dirname]   (create [dirname])", mkdir_do, 6},
    {"cat > ", "  cat > [file]  (write [file], end with [~])", catto_do, 6},
    {"cat ", "  cat [file]    (read [file])", cat_do, 4},
    {"cd ", "  cd [dirname]      (change [work_dir])", cd_do, 3},
    {"ls ", "  ls [dirname]      (list [dirname]'s items)", ls_do, 3},
    {"echo ", "  echo [message]       (print [message])", echo_do, 5},
    {"pwd ", "  pwd               (print [work_dir])", pwd_do, 4},
};

static void default_do(device_t *tty) {
  int offset = 0;
  offset += sprintf(bigbuf + offset, "Unexpected command\n");
  offset += sprintf(bigbuf + offset, "\n             -- help --\n");
  for (int i = sizeof(INFO) / sizeof(struct shellinfo); i >= 0; i--)
    offset += sprintf(bigbuf + offset, "%s\n", INFO[i].script);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

char pwd[1024] = "/";

void shell_task(void *name) {
  device_t *tty = dev_lookup(name);
  char readbuf[128], writebuf[128];

  while (1) {
    sprintf(writebuf, "(%s) $ ", name);
    tty->ops->write(tty, 0, writebuf, strlen(writebuf));
    int nread = tty->ops->read(tty, 0, readbuf, sizeof(readbuf));
    readbuf[nread - 1] = '\0';

    if (!strcmp(readbuf, "ls")) strcpy(readbuf, "ls ");
    if (!strcmp(readbuf, "cd")) strcpy(readbuf, "cd ");
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