#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>
#include <x86.h>
#include <../src/x86/x86-qemu.h>

enum {
  NIL = 0, STARTED = 1, RUNNABLE = 2,
  RUNNING = 3, YIELD = 4, KILLED = 5
};

enum {
  UNLOCKED = 0, LOCKED = 1
};

#define STACK_SIZE 4096

struct task {
  int idx;
  char name[32];
  int state;
  _Context ctx;
  _Area stk;
  struct task *next;
}__attribute__((aligned(32)));

struct spinlock {
  char name[32];
  volatile int locked;
  int cpu;
};

void spin_lock(spinlock_t *lk) {while(_atomic_xchg(&lk->locked, LOCKED));}
void spin_unlock(spinlock_t *lk) {_atomic_xchg(&lk->locked, UNLOCKED);}

struct semaphore {
  char name[32];
  volatile int value;
  struct spinlock lk;
  struct task *head;
};

#define TRACEME

#ifdef TRACEME
  #define TRACE_ENTRY printf("[trace] %s:entry\n", __func__)
  #define TRACE_EXIT printf("[trace] %s:exit\n", __func__)
#else
  #define TRACE_ENTRY ((void)0)
  #define TRACE_EXIT ((void)0)
#endif

#endif
