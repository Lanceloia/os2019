#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>

#define STACK_SIZE 4096

enum STATE{
  NIL = 0, RUNNABLE = 1, RUNNING = 2, YIELD = 3, KILLED = 4
};

struct task {
  char name[32];
  char *stack;
  void (*entry)(void *arg);
  void *arg;
  int state;
};

struct spinlock {
  char name[32];
  int locked;
  int cpu;
};

struct semaphore {
  char name[32];
  int value;
  spinlock_t lock;
};

// Lanceloia lock_t
struct lock{
  int locked;
  int cpu;
};
typedef struct lock lock_t;
void mutex_lock(lock_t *lk);
void mutex_unlock(lock_t *lk);

#endif
