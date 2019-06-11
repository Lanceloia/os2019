#include <klib.h>

#include "../../../include/fs/ext2fs.h"
#include "../../../include/fs/vfs.h"

char readbuf[128], writebuf[128];
char bigbuf[2048] = {};

/* shell command */

static void pwd_do(device_t *tty, char *pwd) {
  sprintf(bigbuf, "%s\n", pwd);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void echo_do(device_t *tty, char *str) {
  sprintf(bigbuf, "%s\n", str);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

extern void vfs_cd(char *dirname, char *pwd, char *out);
extern void ext2_cd(ext2_t *ext2, char *dirname, char *pwd, char *out);
static void cd_do(device_t *tty, char *dirname, char *pwd) {
  int type = vfs_identify_fs(pwd);
  switch (type & ~INTERFACE) {
    case 1:  // vfs
      vfs_cd(dirname, pwd, bigbuf);
      break;
    case 2:  // ext2
      if ((type & INTERFACE) &&
          (!strcmp(dirname, ".") || !strcmp(dirname, "..")))
        vfs_cd(dirname, pwd, bigbuf);
      else
        ext2_cd(vfs_get_real_fs(pwd), dirname, pwd, bigbuf);
      break;
    default:
      assert(0);
  };
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

extern void vfs_ls(char *dirname, char *pwd, char *out);
extern void ext2_ls(ext2_t *ext2, char *dirname, char *out);
static void ls_do(device_t *tty, char *dirname, char *pwd) {
  int type = vfs_identify_fs(pwd);
  switch (type & ~INTERFACE) {
    case 1:  //
      vfs_ls(dirname, pwd, bigbuf);
      break;
    case 2:  // ext2
      ext2_ls(vfs_get_real_fs(pwd), dirname, bigbuf);
      break;
    default:
      assert(0);
  }
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

extern void ext2_mkdir(ext2_t *ext2, char *dirname, int type, char *out);
static void mkdir_do(device_t *tty, char *dirname, char *pwd) {
  int type = vfs_identify_fs(pwd);
  switch (type & ~INTERFACE) {
    case 1:  // vfs
      sprintf(bigbuf, "can't mkdir in vfs.\n", bigbuf);
      break;
    case 2:  // ext2
      ext2_mkdir(vfs_get_real_fs(pwd), dirname, TYPE_DIR, bigbuf);
      break;
    default:
      assert(0);
  };
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void cat_do(device_t *tty, char *path) {
  assert(0);
  int fd = vfs->open(path, RD_ENABLE);
  if (fd == -1) {
    panic("error");
    return;
  }
  while (vfs->read(fd, bigbuf, 1024))
    tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
  if (vfs->close(fd)) {
    panic("error");
    return;
  }
}

static void default_do(device_t *tty) {
  sprintf(bigbuf, "unexpected command\n");
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

void shell_task(void *name) {
  device_t *tty = dev_lookup(name);
  char pwd[256] = "/";
  while (1) {
    sprintf(writebuf, "(%s) $ ", name);
    tty->ops->write(tty, 0, writebuf, strlen(writebuf));
    int nread = tty->ops->read(tty, 0, readbuf, sizeof(readbuf));
    readbuf[nread - 1] = '\0';

    if (!strcmp(readbuf, "pwd"))
      pwd_do(tty, pwd);
    else if (!strncmp(readbuf, "echo ", 5))
      echo_do(tty, readbuf + 5);
    else if (!strncmp(readbuf, "cat ", 4))
      cat_do(tty, readbuf + 4);
    else if (!strncmp(readbuf, "ls ", 3))
      ls_do(tty, readbuf + 3, pwd);
    else if (!strncmp(readbuf, "mkdir ", 6))
      mkdir_do(tty, readbuf + 6, pwd);
    else if (!strncmp(readbuf, "cd ", 3))
      cd_do(tty, readbuf + 3, pwd);
    else
      default_do(tty);
  }
}