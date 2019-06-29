#include <klib.h>

#include "../../include/fs/procfs.h"
#include "../../include/fs/vfs.h"

/* functions
 * copyright: leungjyufung2019@outlook.com
 */

proc_t procs[MAX_PROC];

int procfs_get_idx() {
  int idx = -1;
  for (int i = 0; i < MAX_PROC; i++)
    if (strlen(procs[i].name) == 0) {
      idx = i;
      break;
    }
  return idx;
}

#define procfs_add(idx, name, mode) \
  do {                              \
    idx = procfs_get_idx();         \
    strcpy(procs[idx].name, name);  \
    procs[idx].cpu_number = -1;     \
    procs[idx].schduel_times = 0;   \
    procs[idx].inode = idx;         \
    procs[idx].mode = mode;         \
    printf("fuck: %s\n", name);     \
  } while (0)

void *procfs_addproc(const char *name) {
  int idx;
  procfs_add(idx, name, TYPE_FILE);
  return &procs[idx];
}

void procfs_schdule(void *proc) {
  proc_t *_proc = (proc_t *)proc;
  _proc->cpu_number = _cpu();
  _proc->schduel_times++;
}

int procfs_init(filesystem_t *fs, const char *name, device_t *dev) {
  int dot, ddot, mode = TYPE_DIR;
  procfs_add(dot, ".", mode);
  procfs_add(ddot, "..", mode);
  return 1;
}

int procfs_readdir(filesystem_t *fs, int ridx, int kth, vinode_t *buf) {
  printf("here!\n");
  for (int k = 0, cnt = 0; k < MAX_PROC; k++) {
    if (++cnt == kth) {
      strcpy(buf->name, procs[k].name);
      buf->ridx = procs[k].inode;
      buf->mode = procs[k].mode;
      return 1;
    }
  }
  return 0;
}

/*

void procfs_cd(char *dirname, char *pwd, char *out) {
  sprintf(out, "can't cd here\n");
  return;
}

void procfs_ls(char *dirname, char *out) {
  int offset = sprintf(out, "");
  if (!strcmp(dirname, ".")) {
    offset += sprintf(out + offset, "proc            cpu      schdule
times\n"); for (int i = 0; i < MAX_PROC; i++) { if (strlen(procs[i].name) ==
0) continue; offset += sprintf(out + offset, "%s", procs[i].name); for (int j
= 0; j < 15 - strlen(procs[i].name); j++) offset += sprintf(out + offset,
"%c", ' '); offset += sprintf(out + offset, " %d        ",
procs[i].cpu_number); offset += sprintf(out + offset, "%d",
procs[i].schduel_times); offset += sprintf(out + offset, "\n");
    }
  } else {
    offset += sprintf(out + offset, "only support 'ls .'\n");
  }
}

*/