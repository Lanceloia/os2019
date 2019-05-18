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

enum {
  NONE_CPU = -1
};

#define SLEEP(time) for(volatile int __itr__ = 0; __itr__ < (time); __itr__++)
#define MAX_TASK 32
#define STK_SIZE 4096

typedef intptr_t naivelock_t; 
#define naivelock_lock(locked) { while(_atomic_xchg((&locked), LOCKED)) SLEEP(16); }
#define naivelock_unlock(locked) { _atomic_xchg((&locked), UNLOCKED); }

struct task {
  int idx;
  int state;
  _Context ctx;
  _Area stk;
  char name[32];
  char stack[STK_SIZE];
}__attribute__((aligned(32)));

struct spinlock {
  char name[32];
  volatile int locked;
  int cpu;
};

struct semaphore {
  char name[32];
  volatile int value;
  struct spinlock lk;

  struct task *stack[MAX_TASK];
  int top;
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
