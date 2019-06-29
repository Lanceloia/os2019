#include <klib.h>

#include "../../include/fs/procfs.h"
#include "../../include/fs/vfs.h"

/* functions
 * copyright: leungjyufung2019@outlook.com
 */

proc_t procs[MAX_PROC];
int total_proc = 0;

#define procfs_add(idx, _name, mode) \
  do {                               \
    idx = total_proc++;              \
    strcpy(procs[idx].name, _name);  \
    procs[idx].cpu_number = -1;      \
    procs[idx].schduel_times = 0;    \
    procs[idx].inode = idx;          \
    procs[idx].mode = mode;          \
  } while (0)

int is_initialized = 0;

void *procfs_addproc(const char *name) {
  if (!is_initialized) return NULL;
  if (total_proc == MAX_PROC - 1) {
    printf("Cannot create more proc! \n");
    return NULL;
  }
  int idx, mode = TYPE_FILE;
  procfs_add(idx, name, mode);
  return &procs[idx];
}

void procfs_schdule(void *oldproc, void *newproc) {
  if (!newproc) return;
  proc_t *oproc = (proc_t *)oldproc;
  oproc->cpu_number = -1;

  proc_t *nproc = (proc_t *)newproc;
  nproc->cpu_number = _cpu();
  nproc->schduel_times++;
}

int procfs_init(filesystem_t *fs, const char *name, device_t *dev) {
  is_initialized = 1;
  int dot, ddot, mode = TYPE_DIR;
  char dotname[] = ".", ddotname[] = "..";
  procfs_add(dot, dotname, mode);
  procfs_add(ddot, ddotname, mode);
  return 1;
}

int procfs_readdir(filesystem_t *fs, int ridx, int kth, vinode_t *buf) {
  printf("here!\n");
  for (int k = 0, cnt = 0; k < total_proc; k++) {
    if (++cnt == kth) {
      strcpy(buf->name, procs[k].name);
      buf->ridx = procs[k].inode;
      buf->mode = procs[k].mode;
      // printf("find %s\n", buf->name);
      return 1;
    }
  }
  return 0;
}

ssize_t procfs_read(int ridx, uint64_t offset, char *buf) {
  if (offset != 0) return 0;
  int ret = sprintf(buf, "  name: %s\n", procs[ridx].name);
  ret += sprintf(buf + ret, "  cpu: %d\n", procs[ridx].cpu_number);
  ret += sprintf(buf + ret, "  schduel: %d\n", procs[ridx].schduel_times);
  return ret;
}
