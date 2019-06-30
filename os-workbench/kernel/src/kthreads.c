// #define _LANCELOIA_DEBUG_

#include <common.h>
#include <klib.h>
#include <kthreads.h>

static void kmt_init();
static int kmt_create(task_t *, const char *, void (*)(void *), void *);
static void kmt_teardown(task_t *);
static void kmt_spin_init(spinlock_t *, const char *);
static void kmt_spin_lock(spinlock_t *);
static void kmt_spin_unlock(spinlock_t *);
static void kmt_sem_init(sem_t *, const char *, int);
static void kmt_sem_wait(sem_t *);
static void kmt_sem_signal(sem_t *);

static int tasks_size = 0;
static task_t *tasks[MAX_TASK];
static task_t *current_tasks[MAX_CPU];

#define current (current_tasks[_cpu()])

extern void *procfs_addproc(const char *proc);
extern void procfs_schdule(void *oldproc, void *newproc);

static _Context *kmt_context_save_switch(_Event ev, _Context *ctx) {
  if (current) current->ctx = *ctx;

  task_t *otask = current, *ntask = NULL;
  if (current && current->state == RUNNING) current->state = RUNNABLE;

  int idx = (!current) ? 0 : current->idx;
  while (1) {
    idx = (idx + 1) % tasks_size;
    if (!tasks[idx]) continue;
    if (tasks[idx]->state == STARTED || tasks[idx]->state == RUNNABLE) break;
  }
  current = tasks[idx];
  current->state = RUNNING;

  ntask = current;
  procfs_schdule(otask->proc, ntask->proc);
  return &current->ctx;
}

/* tasks */

static int get_tasks_idx() {
  int ret = tasks_size;
  for (int idx = 0; idx < tasks_size; idx++)
    if (tasks[idx] == NULL) ret = idx;
  return ret;
}

static void tasks_insert(task_t *x) {
  naivelock_lock(&tasks_mutex);
  x->idx = get_tasks_idx();
  if (x->idx == tasks_size) tasks_size++;
  tasks[x->idx] = x;
  naivelock_unlock(&tasks_mutex);
}

static void tasks_remove(task_t *x) {
  naivelock_lock(&tasks_mutex);
  tasks[x->idx] = NULL;
  naivelock_unlock(&tasks_mutex);
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg),
                      void *arg) {
  task->state = STARTED;
  strcpy(task->name, name);
  task->stk.start = task->stack;
  task->stk.end = task->stk.start + STK_SIZE;
  task->ctx = *(_kcontext(task->stk, entry, arg));
  task->proc = procfs_addproc(name);
  tasks_insert(task);
#ifdef _LANCELOIA_DEBUG_
  printf("[task] created [%s] [%d]\n", task->name, task->idx);
#endif
  return 0;
}

static void kmt_teardown(task_t *task) {
  tasks_remove(task);
#ifdef _LANCELOIA_DEBUG_
  printf("[task] removed [%s] [%d]\n", task->name, task->idx);
#endif
}

static void kmt_create_wait() {
  char buf[128];
  for (int i = 0; i < _ncpu(); i++) {
    sprintf(buf, "wait%d", i);
    kmt_create(&wait[i], buf, task_wait, NULL);
  }
}

static void kmt_init() {
  os->on_irq(INT32_MAX, _EVENT_NULL, kmt_context_save_switch);
  // kmt_spin_init(&tasks_mutex, "tasks-mutex");
  // kmt_spin_init(&current_tasks_mutex, "current-tasks-mutex");
  kmt_create_wait();
}

/* spin */

static void kmt_spin_init(spinlock_t *lk, const char *name) {
  strcpy(lk->name, name);
  lk->locked = UNLOCKED;
  lk->cpu = NONE_CPU;
#ifdef _LANCELOIA_DEBUG_
  printf("[spinlock] created [%s]\n", lk->name);
#endif
}

static void kmt_spin_lock(spinlock_t *lk) {
#ifdef _LANCELOIA_DEBUG_
  if (holding(lk)) panic("locked");
#endif
  pushcli();
  while (_atomic_xchg(&lk->locked, LOCKED)) SLEEP(256);
  lk->cpu = _cpu();
  __sync_synchronize();
}

static void kmt_spin_unlock(spinlock_t *lk) {
#ifdef _LANCELOIA_DEBUG_
  if (!holding(lk)) panic("unlocked");
#endif
  lk->cpu = NONE_CPU;
  _atomic_xchg(&(lk->locked), UNLOCKED);
  popcli();
  __sync_synchronize();
}

/* sem */

static void sem_push(sem_t *sem, task_t *task) {
  assert(sem->lk.locked);
  sem->que[sem->tail] = task;
  sem->tail = (sem->tail + 1) % MAX_TASK;
}

static task_t *sem_pop(sem_t *sem) {
  assert(sem->lk.locked);
  int _head = sem->head;
  sem->head = (sem->head + 1) % MAX_TASK;
  return sem->que[_head];
}

static void sleep(sem_t *sem) {
  current->state = YIELD;
  sem_push(sem, current);
  kmt_spin_unlock(&sem->lk);
  _yield();
}

static void wakeup(sem_t *sem) {
  if (sem->head == sem->tail) {
    _halt(1);
  }
  task_t *task = sem_pop(sem);
  task->state = RUNNABLE;
  kmt_spin_unlock(&sem->lk);
}

static void kmt_sem_init(sem_t *sem, const char *name, int value) {
  strcpy(sem->name, name);
  sem->value = value;
  sem->head = sem->tail = 0;
  kmt_spin_init(&sem->lk, name);
#ifdef _LANCELOIA_DEBUG_
  printf("[sem] created [%s] [%d]\n", sem->name, sem->value);
#endif
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

MODULE_DEF(kmt){
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

#ifdef _LANCELOIA_DEBUG_
#undef _LANCELOIA_DEBUG_
#endif
