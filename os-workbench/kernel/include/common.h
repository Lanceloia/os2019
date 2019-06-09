#ifndef __COMMON_H__
#define __COMMON_H__

#include <../src/x86/x86-qemu.h>
#include <kernel.h>
#include <nanos.h>
#include <x86.h>

enum { UNLOCKED = 0, LOCKED = 1 };

enum { NONE_CPU = -1 };

#define KB *1024
#define MB *1024 * 1024

#define MAX_TASK 32
#define NAME_lENGTH 32
#define MAX_FILE 32
#define STK_SIZE (4 KB)

struct task {
  int idx;
  int state;
  _Context ctx;
  _Area stk;
  struct file *files[MAX_FILE];
  char name[NAME_lENGTH];
  char stack[STK_SIZE];
} __attribute__((aligned(32)));

struct spinlock {
  char name[NAME_lENGTH];
  volatile int locked;
  int cpu;
};

struct semaphore {
  char name[NAME_lENGTH];
  volatile int value;
  struct spinlock lk;
  struct task *que[MAX_TASK];
  int head, tail;
};

typedef intptr_t naivelock_t;
#define SLEEP(time) for (volatile int __itr__ = 0; __itr__ < (time); __itr__++)
#define naivelock_lock(lock) \
  while (_atomic_xchg(lock, LOCKED)) SLEEP(16);
#define naivelock_unlock(lock) _atomic_xchg(lock, UNLOCKED);

#define TRACEME
#ifdef TRACEME
#define TRACE_ENTRY printf("[trace] %s:entry\n", __func__)
#define TRACE_EXIT printf("[trace] %s:exit\n", __func__)
#else
#define TRACE_ENTRY ((void)0)
#define TRACE_EXIT ((void)0)
#endif

#endif
