#ifndef __PROCFS_H__
#define __PROCFS_H__

#include <devices.h>

struct proc {
  /* inode, 64 bytes */
  char name[16];
  uint32_t cpu_number;
  uint32_t schduel_times;
};

#endif