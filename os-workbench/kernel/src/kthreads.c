#include <common.h>
#include <klib.h>
#include <kthreads.h>

/*
static void kmt_create_wait() {
  for(int i = 0; i < _ncpu(); i++) {
  strcpy(wait[i].name, "wait");
    wait[i].stk.start = pmm->alloc(STACK_SIZE);
    wait[i].stk.end = wait[i].stk.start + STACK_SIZE;
    wait[i].ctx = *(_kcontext(wait[i].stk, task_wait, NULL));
    wait[i].state = STARTED;
  }
}
*/

#define MAX_TASK 32

static int tasks_size = 0;
static task_t *tasks[MAX_TASK];
static task_t *current_tasks[MAX_CPU];

#define current (current_tasks[_cpu()])

static _Context *kmt_context_save(_Event ev, _Context *ctx) {
  if (current)
    current->ctx = *ctx;
  return NULL;
}

static _Context *kmt_context_switch(_Event ev, _Context *ctx) {
  if(current && current->state == RUNNING)
    current->state = RUNNABLE;
  //current = NULL;
  do {
    if (!current || current->idx + 1 == tasks_size)
      current = tasks[0];
    else
      current = tasks[current->idx + 1];
    // printf("%c ", _cpu() + 'a');
  } while (!(current->state == STARTED || current->state == RUNNABLE));
 
  current->state = RUNNING;
  return &current->ctx;
}

static void tasks_insert(task_t *x) {
  kmt_spin_lock(&tasks_mutex);
  tasks[tasks_size++] = x;
  kmt_spin_unlock(&tasks_mutex);
}

static void tasks_remove(task_t *x) {
  kmt_spin_lock(&tasks_mutex);
  assert(0);
  kmt_spin_unlock(&tasks_mutex);
}

static void kmt_init() {
  os->on_irq(INT32_MIN, _EVENT_NULL, kmt_context_save);
  os->on_irq(INT32_MAX, _EVENT_NULL, kmt_context_switch);
  kmt_spin_init(&tasks_mutex, "tasks-mutex");
  kmt_spin_init(&current_tasks_mutex, "current-tasks-mutex");
  for(int i = 0; i < _ncpu(); i++) {
    kmt_create(&wait[i], "wait", task_wait, NULL);
  }

  // kmt_create_wait();
}

/* tasks
 * create(), teardown()
 */

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  task->idx = tasks_size;
  task->state = STARTED;
  strcpy(task->name, name);
  task->stk.start = task->stack;
  task->stk.end = task->stk.start + 4096;
  task->ctx = *(_kcontext(task->stk, entry, arg));
  tasks_insert(task);
  printf("[task] created [%s]\n", task->name);
  return 0; 
}

static void kmt_teardown(task_t *task) {
  TRACE_ENTRY;
  assert(0);
  tasks_remove(current);
  TRACE_EXIT;
}

/* spinlock
 * spin_init(), spin_lock(), spin_unlock()
 */

static void kmt_spin_init(spinlock_t *lk, const char *name) {
  strcpy(lk->name, name);
  lk->locked = UNLOCKED;
  lk->cpu = NONE_CPU;
  printf("[spinlock] created [%s]\n", lk->name);
}

static void kmt_spin_lock(spinlock_t *lk) {
  if (holding(lk)) {
    printf("\nERROR: spin_lock error! lk->name: %s\n", 
      lk->name);
    _halt(1);
  }
  pushcli();
  while(_atomic_xchg(&lk->locked, LOCKED))
    for(volatile int i = 0; i < 2048; i++);
  lk->cpu = _cpu();
  __sync_synchronize();
  //for(volatile int i = 0; i < 15000; i++);
}

static void kmt_spin_unlock(spinlock_t *lk) {
  for(volatile int i = 0; i < 15000; i++);
  if (!holding(lk)) {
    printf("\nERROR: spin_unlock error! lk->name: %s\n", 
      lk->name);

    _halt(1);
  }
  lk->cpu = NONE_CPU;
  _atomic_xchg(&(lk->locked), UNLOCKED);

  popcli();
  __sync_synchronize();
}

/* semaphore
 * sem_init(), sem_wait(), sem_signal()
 */

static void sem_push(sem_t *sem, task_t *task) {
  assert(sem->lk.locked);
  sem->stack[(sem->top)++] = task;
}

static task_t *sem_pop(sem_t *sem) {
  assert(sem->lk.locked);
  return sem->stack[--(sem->top)];
}

static void kmt_sem_init(sem_t *sem, const char *name, int value) {
  strcpy(sem->name, name);
  sem->value = value;
  sem->top = 0;
  kmt_spin_init(&sem->lk, name);
  printf("[log] created semaphore [%s]\n", sem->name);
}

static void sleep (sem_t *sem) {
  current->state = YIELD;
  sem_push(sem, current);
  kmt_spin_unlock(&sem->lk);
  _yield();
}

static void wakeup (sem_t *sem) {
  if (sem->top == 0) {
    _halt(1);
  }
  task_t *task = sem_pop(sem);
  task->state = RUNNABLE;
  kmt_spin_unlock(&sem->lk);
}

static void kmt_sem_wait(sem_t *sem) {
  kmt_spin_lock(&sem->lk);
  sem->value--;
  if (sem->value < 0)
    sleep(sem);
  else
    kmt_spin_unlock(&sem->lk);
}

static void kmt_sem_signal(sem_t *sem) {
  kmt_spin_lock(&sem->lk);
  sem->value++;
  if (sem->value <= 0)
    wakeup(sem);
  else
    kmt_spin_unlock(&sem->lk);
}

MODULE_DEF(kmt) {
  .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
};


