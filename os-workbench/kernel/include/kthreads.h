#ifndef __KTHREADS_H__
#define __KTHREADS_H__

 static void kmt_init();
 static int kmt_create(task_t *, const char *, void (*)(void *), void *);
 static void kmt_teardown(task_t *);
 static void kmt_spin_init(spinlock_t *, const char *);
 static void kmt_spin_lock(spinlock_t *);
 static void kmt_spin_unlock(spinlock_t *);
 static void kmt_sem_init(sem_t *, const char *, int);
 static void kmt_sem_wait(sem_t *);
 static void kmt_sem_signal(sem_t *);

#define MAX_TASK 32
#define STK_SIZE 4096

enum {
  NIL = 0, STARTED = 1, RUNNABLE = 2,
  RUNNING = 3, YIELD = 4, KILLED = 5
};

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

  struct task *que[MAX_TASK];
  int head, tail;
};

/* data-structure
 * tasks_list_head, current_tasks
 */

spinlock_t tasks_mutex;
spinlock_t current_tasks_mutex;

/* spinlock-manage
 * pushcli(), popcli(), holding()
 */

naivelock_t cli_mutex = UNLOCKED;
volatile int ncli[MAX_CPU] = {} , intena[MAX_CPU] = {};

void pushcli() {
  cli();
  if (ncli[_cpu()] == 0)
    intena[_cpu()] = get_efl() & FL_IF;
  naivelock_lock(cli_mutex);
  ncli[_cpu()] ++;
  naivelock_unlock(cli_mutex);
}

void popcli() {
  naivelock_lock(cli_mutex);
  ncli[_cpu()] --;
  naivelock_unlock(cli_mutex);
  if (ncli[_cpu()] == 0 && intena[_cpu()])
    sti();
}

int holding(spinlock_t *lk) {
  return lk->locked && lk->cpu == _cpu();
}

task_t wait[MAX_CPU] = {};
void task_wait(void *arg) {
  while (1) ;
}

#endif
