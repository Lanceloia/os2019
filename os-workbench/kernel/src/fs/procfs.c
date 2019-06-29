#include <klib.h>

#include "../../include/fs/procfs.h"
#include "../../include/fs/vfs.h"

/* functions
 * copyright: leungjyufung2019@outlook.com
 */

proc_t procs[MAX_PROC];
int running[MAX_CPU];
int total_proc = 4;
uint64_t mem_total = 0;
uint64_t mem_used = 0;

#define procfs_add(idx, _name)      \
  do {                              \
    idx = total_proc++;             \
    strcpy(procs[idx].name, _name); \
    procs[idx].inode = idx - 4;     \
    procs[idx].cpu_number = -1;     \
    procs[idx].sche_times = 0;      \
    procs[idx].memo_size = 0;       \
  } while (0)

void *procfs_addproc(const char *name) {
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
    running[_cpu()] = nproc->inode;
  }
}

void procfs_mem_trace(uint64_t size, int mode) {
  switch (mode) {
    case 0:  // plus
      mem_size += size;
      break;
    case 1:  // minus
      mem_size -= size;
      break;
    default:
      assert(0);
      break;
  }
}

int procfs_init(filesystem_t *fs, const char *name, device_t *dev) {
  int _total_proc = total_proc, idx;
  total_proc = 0;
  char *names[] = {".", "..", "cpuinfo", "meminfo"};
  procfs_add(idx, names[0]);
  procfs_add(idx, names[1]);
  procfs_add(idx, names[2]);
  procfs_add(idx, names[3]);
  total_proc = _total_proc;
  return 1;
}

int procfs_readdir(filesystem_t *fs, int ridx, int kth, vinode_t *buf) {
  // printf("here!\n");
  for (int k = 0, cnt = 0; k < total_proc; k++) {
    if (++cnt == kth) {
      buf->ridx = k;
      if (k == 0 || k == 1) {
        strcpy(buf->name, procs[k].name);
        buf->mode = TYPE_DIR;
      } else if (k == 2 || k == 3) {
        strcpy(buf->name, procs[k].name);
        buf->mode = TYPE_FILE;
      } else {
        sprintf(buf->name, "%d", procs[k].inode);
        buf->mode = TYPE_FILE;
      }
      return 1;
    }
  }
  return 0;
}

ssize_t procfs_read(int ridx, uint64_t offset, char *buf) {
  if (offset != 0) return 0;

  int ret = 0;

  if (ridx == 2) {
    for (int i = 0; i < _ncpu(); i++) {
      int k = running[i] + 4;
      ret += sprintf(buf + ret, "  [cpu %d]: ", procs[k].cpu_number);
      ret += sprintf(buf + ret, "%s\n", procs[k].name);
    }
  } else if (ridx == 3) {
    ret += sprintf(buf + ret, "  [total]: %d KB\n", mem_total / 1024);
    ret += sprintf(buf + ret, "  [used]: %d KB\n", mem_used / 1024);
    ret +=
        sprintf(buf + ret, "  [free]: %d KB\n", (mem_total - mem_used) / 1024);
  } else {
    ret += sprintf(buf + ret, "  pid: %d\n", procs[ridx].inode);
    ret += sprintf(buf + ret, "  name: %s\n", procs[ridx].name);
    ret += sprintf(buf + ret, "  cpu: %d\n", procs[ridx].cpu_number);
    ret += sprintf(buf + ret, "  mem: %d\n", procs[ridx].memo_size);
    ret += sprintf(buf + ret, "  schduel_times: %d\n", procs[ridx].sche_times);
  }
  return ret;
}
