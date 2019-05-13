#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>
#include <x86.h>
#include <../src/x86/x86-qemu.h>

enum {
  NIL = 0, STARTED = 1, RUNNABLE = 2,
  RUNNING = 3, YIELD = 4, KILLED = 5
}; // state

enum {
  UNLOCKED = 0, LOCKED = 1
}; // lock

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
  int locked;
  int cpu;
};

struct semaphore {
  char name[32];
  int value;
  struct task *slptsk_head;
};

// Lanceloia lock_t
struct lock{
  int locked;
  int cpu;
};

typedef struct lock lock_t;

void mutex_lock(lock_t *lk);

void mutex_unlock(lock_t *lk);

#define TRACEME

#ifdef TRACEME
  #define TRACE_ENTRY printf("[trace] %s:entry\n", __func__)
  #define TRACE_EXIT printf("[trace] %s:exit\n", __func__)
#else
  #define TRACE_ENTRY ((void)0)
  #define TRACE_EXIT ((void)0)
#endif

#endif
