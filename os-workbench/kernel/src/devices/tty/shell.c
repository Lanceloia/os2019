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

char absolutely_path[256];

static void ls_do(device_t *tty, char *dirname, char *pwd) {
  extern void vfs_ls(char *dirname);
  strcpy(absolutely_path, pwd);
  strcat(absolutely_path, dirname);
  vfs_ls(dirname);
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

/*
extern void ext2_mkdir(ext2_t *ext2, char *dirname, int type, char *out);
static void mkdir_do(device_t *tty, char *dirname, char *pwd) {
  int type = vfs_identify_fs(pwd);
  int type2 = TYPE_DIR;
  for (int i = 0; i < strlen(dirname); i++)
    if (dirname[i] == '.') type2 = TYPE_FILE;
  switch (type & ~INTERFACE) {
    case 2:  // ext2
      ext2_mkdir(vfs_get_real_fs(pwd), dirname, type2, bigbuf);
      break;
    default:
      sprintf(bigbuf, "can't mkdir here.\n", bigbuf);
      break;
  };
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

extern void ext2_rmdir(ext2_t *ext2, char *dirname, char *out);
static void rmdir_do(device_t *tty, char *dirname, char *pwd) {
  int type = vfs_identify_fs(pwd);
  switch (type & ~INTERFACE) {
    case 2:  // ext2
      ext2_rmdir(vfs_get_real_fs(pwd), dirname, bigbuf);
      break;
    default:
      sprintf(bigbuf, "can't rmdir here.\n", bigbuf);
      break;
  };
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

extern void ext2_cat(ext2_t *ext2, char *dirname, char *out);
static void cat_do(device_t *tty, char *dirname, char *pwd) {
  int type = vfs_identify_fs(pwd);
  switch (type & ~INTERFACE) {
    case 2:  // ext2
      ext2_cat(vfs_get_real_fs(pwd), dirname, bigbuf);
      break;
    default:
      sprintf(bigbuf, "can't cat here.\n", bigbuf);
      break;
  };
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

static void default_do(device_t *tty) {
  sprintf(bigbuf, "unexpected command\n");
  tty->ops->write(tty, 0, bigbuf, strlen(bigbuf));
}

extern int vfsdirs_alloc(const char *name, int parent, int type, int fs_idx);

*/

// extern int dev_dir;
// extern int total_dev_cnt;

char pwd[256] = "/";

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

    /*
    if (!strcmp(readbuf, "pwd"))
      pwd_do(tty, pwd);
    else if (!strncmp(readbuf, "echo ", 5))
      echo_do(tty, readbuf + 5);
    else if (!strncmp(readbuf, "cat ", 4))
      cat_do(tty, readbuf + 4, pwd);
    */
    if (!strncmp(readbuf, "echo ", 5)) echo_do(tty, readbuf + 5);
    if (!strncmp(readbuf, "ls ", 3)) ls_do(tty, readbuf + 3, pwd);
    /*
    else if (!strncmp(readbuf, "mkdir ", 6))
      mkdir_do(tty, readbuf + 6, pwd);
    else if (!strncmp(readbuf, "rmdir ", 6))
      rmdir_do(tty, readbuf + 6, pwd);
    else if (!strncmp(readbuf, "cd ", 3))
      cd_do(tty, readbuf + 3, pwd);
    else
      default_do(tty);
    */
  }
}