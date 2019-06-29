#ifndef __PROCFS_H__
#define __PROCFS_H__

#include <devices.h>

struct proc {
  /* proc, 32 bytes */
  uint32_t inode;
  uint32_t cpu_number;
  uint32_t schduel_times;
  uint32_t mode;
  char name[16];
};

typedef struct proc proc_t;

#define MAX_PROC 16

#endif