#include <devices.h>

char readbuf[128], writebuf[128];
char bigbuf[2048] = {};

/* shell command */
#include <fs.h>

uint32_t current_dir;
uint32_t current_dir_name_len;
char current_dir_name[128];

static void echo_do(device_t *tty, char *str) {
  sprintf(bigbuf, "%s\n", str);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

/*
static void cd_do(device_t *tty, char *dirname) {
  extern void ext2_cd(fs_t * fs, char *dirname, char *buf);
}
*/

static void ls_do(device_t *tty, char *dirname) {}

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
  while (1) {
    sprintf(writebuf, "(%s) $ ", name);
    tty->ops->write(tty, 0, writebuf, strlen(writebuf));
    int nread = tty->ops->read(tty, 0, readbuf, sizeof(readbuf));
    readbuf[nread - 1] = '\0';

    if (strncmp(readbuf, "echo ", 5) == 0)
      echo_do(tty, readbuf + 5);
    else if (strncmp(readbuf, "cat ", 4) == 0)
      cat_do(tty, readbuf + 4);
    else
      default_do(tty);
  }
}