#include <klib.h>

#include "../../include/fs/procfs.h"
#include "../../include/fs/vfs.h"

/* functions
 * copyright: leungjyufung2019@outlook.com
 */

proc_t procs[MAX_PROC];
int total_proc = 0;

#define procfs_add(idx, _name)      \
  do {                              \
    idx = total_proc++;             \
    strcpy(procs[idx].name, _name); \
    procs[idx].inode = idx - 1;     \
    procs[idx].cpu_number = -1;     \
    procs[idx].sche_times = 0;      \
    procs[idx].memo_size = 0;       \
  } while (0)

int is_initialized = 0;

void *procfs_addproc(const char *name) {
  if (!is_initialized) return NULL;
  if (total_proc == MAX_PROC - 1) {
    printf("Cannot create more proc! \n");
    return NULL;
  }
  int idx;
  procfs_add(idx, name);
  return &procs[idx];
}

void procfs_schdule(void *oldproc, void *newproc) {
  if (oldproc) {
    proc_t *oproc = (proc_t *)oldproc;
    oproc->cpu_number = -1;
  }

  if (newproc) {
    proc_t *nproc = (proc_t *)newproc;
    nproc->cpu_number = _cpu();
    nproc->sche_times++;
  }
}

int procfs_init(filesystem_t *fs, const char *name, device_t *dev) {
  is_initialized = 1;
  int dot, ddot;
  char dotname[] = ".", ddotname[] = "..";
  procfs_add(dot, dotname);
  procfs_add(ddot, ddotname);
  return 1;
}

int procfs_readdir(filesystem_t *fs, int ridx, int kth, vinode_t *buf) {
  printf("here!\n");
  for (int k = 0, cnt = 0; k < total_proc; k++) {
    if (++cnt == kth) {
      sprintf(buf->name, "%d", procs[k].inode);
      buf->ridx = procs[k].inode;
      buf->mode = procs[k].inode < 1 ? TYPE_DIR : TYPE_FILE;
      // printf("find %s\n", buf->name);
      return 1;
    }
  }
  return 0;
}

ssize_t procfs_read(int ridx, uint64_t offset, char *buf) {
  if (offset != 0) return 0;
  int ret = sprintf(buf, "  pid: %s\n", procs[ridx].inode);
  ret += sprintf(buf + ret, "  name: %s\n", procs[ridx].name);
  ret += sprintf(buf + ret, "  cpuinfo: %d\n", procs[ridx].cpu_number);
  ret += sprintf(buf + ret, "  memory_used: %d\n", procs[ridx].memo_size);
  ret += sprintf(buf + ret, "  schduel_times: %d\n", procs[ridx].sche_times);
  return ret;
}
