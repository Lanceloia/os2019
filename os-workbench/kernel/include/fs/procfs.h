#ifndef __PROCFS_H__
#define __PROCFS_H__

#include <devices.h>

struct proc {
  /* proc, 32 bytes */
  uint32_t cpu_number;
  uint32_t schduel_times;
  char name[16];
  char pad[8];
};

typedef struct proc proc_t;

#define MAX_PROC 16

#endif