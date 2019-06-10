#include <klib.h>
#include <simext2.h>

char readbuf[128], writebuf[128];
char bigbuf[2048] = {};

/* shell command */
#include <fs.h>

static void pwd_do(device_t *tty, char *pwd) {
  tty->ops->write(tty, 0, pwd, strlen(pwd));
}

static void echo_do(device_t *tty, char *str) {
  tty->ops->write(tty, 0, str, strlen(str));
}

/*
static void cd_do(device_t *tty, char *dirname) {
  extern void ext2_cd(fs_t * fs, char *dirname, char *buf);
}
*/

extern void ext2_ls(ext2_t *ext2, char *dirname, char *out);
static void ls_do(device_t *tty, char *dirname) {
  ext2_ls(vfs->get_fs(0)->fs, dirname, bigbuf);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

extern void ext2_mkdir(ext2_t *ext2, char *dirname, int type, char *out);
static void mkdir_do(device_t *tty, char *dirname) {
  int type = TYPE_DIR, name_len = strlen(dirname);
  for (int i = 0; i < name_len; i++)
    if (dirname[i] == '.') type = TYPE_FILE;  // point
  ext2_mkdir(vfs->get_fs(0)->fs, dirname, type, bigbuf);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void cat_do(device_t *tty, char *path) {
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
      ls_do(tty, readbuf + 3);
    else if (!strncmp(readbuf, "mkdir ", 6))
      mkdir_do(tty, readbuf + 6);

    else
      default_do(tty);
  }
}