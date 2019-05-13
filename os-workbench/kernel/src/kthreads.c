#include <common.h>
#include <klib.h>
#include <kthreads.h>

/*
#define MAX_TASK 128

#define LENGTH(arr) (sizeof(arr)/sizeof(arr[0]))

int tasks_sz;

task_t tasks[MAX_TASK];

task_t *current_task[MAX_CPU];

static void tasks_push_back(task_t *x) {
x->idx = tasks_sz;
tasks[tasks_sz] = *x;
tasks_sz++;
}

static void tasks_remove(task_t *x) {
int idx = x->idx;
printf("sz: %d, %d -> %d\n",
tasks_sz, tasks[tasks_sz - 1].idx, tasks[idx].idx);
tasks[idx] = tasks[tasks_sz - 1];
tasks[idx].idx = idx;
tasks_sz--;
}
 */

task_t *current_task[MAX_CPU];

#define current (current_task[_cpu()])

static _Context *kmt_context_save(_Event ev, _Context *ctx) {
  return NULL; 
}

static _Context *kmt_context_switch(_Event ev, _Context *ctx) {
  if (current)
    current->ctx = *ctx;
  if (!current || current->next == NULL)
    current = tasks_head;
  else
    current = current->next;
  return &current->ctx;
}

static void kmt_init() {
  TRACE_ENTRY;
  os->on_irq(INT32_MIN, _EVENT_NULL, kmt_context_save);
  os->on_irq(INT32_MAX, _EVENT_NULL, kmt_context_switch);

  TRACE_EXIT;
}

/* KMT threads manage
 * 
 * kmt_create: create a thread named [name], the function is [*entry], start it with argument [arg]
 * kmt_teardown: reclaim the resource of thread saved at [task]
 */

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  TRACE_ENTRY;

  strcpy(task->name, name);
  task->stk.start = pmm->alloc(STACK_SIZE);
  task->stk.end = task->stk.start + STACK_SIZE;
  task->ctx = *(_kcontext(task->stk, entry, arg));
  task->state = STARTED;

  tasks_push_back(task);
  printf("[log] created task [%s], prot [%x]\n",
      task->name, task->ctx.prot);
  TRACE_EXIT;
  return 0; 
}

static void kmt_teardown(task_t *task) {
  TRACE_ENTRY;
  assert(0);
  TRACE_EXIT;
}

/* KMT spin-lock
 * 
 * kmt_spin_init: initialize the new lock saved at [lk], name it with [name]
 * kmt_spin_lock: try to lock the [lk]
 * kmt_spin_unlock: try to unlock the [lk]
 */

static void kmt_spin_init(spinlock_t *lk, const char *name) {
  strcpy(lk->name, name);
  lk->locked = UNLOCKED;
  lk->cpu = -1;
  printf("[log] created spinlock [%s]\n", lk->name);
}

static void pushcli();

static void popcli();

static int holding(spinlock_t *lk);

static void kmt_spin_lock(spinlock_t *lk) {
  pushcli();
  if (holding(lk))
    panic("spin_lock");
  while (_atomic_xchg(&lk->locked, LOCKED) != UNLOCKED);
  __sync_synchronize();
  lk->cpu = _cpu();
  // getcallerpcs ...
}

static void kmt_spin_unlock(spinlock_t *lk) {
  if (!holding(lk))
    panic("spin_unlock");
  lk->cpu = -1;
  __sync_synchronize();
  asm volatile("movl $0, %0" : 
      "=m" (lk->locked) : ); // 0 for UNLOCKED
  popcli();
}

/* KMT semaphore
 *
 * I'm lazy
 */

static void kmt_sem_init(sem_t *sem, const char *name, int value) {
  // TRACE_ENTRY;
  strcpy(sem->name, name);
  sem->value = value;
  sem->slptsk_head = NULL;
  printf("[log] created semaphore [%s]\n", sem->name);
}

static void sleep (sem_t *sem) {
  // return;
  TRACE_ENTRY;
  printf("sleep sem[%s] task[%s]\n", sem->name, current->name);
  tasks_remove(current);

  current->next = sem->slptsk_head;
  sem->slptsk_head = current;
  _yield();
  //panic("should not be here!");
}

static void wakeup (sem_t *sem) {
  TRACE_ENTRY;
  if (sem->slptsk_head != NULL) {
    task_t *task = sem->slptsk_head;
    sem->slptsk_head = sem->slptsk_head->next;
    task->state = RUNNABLE;
    tasks_push_back(task);
    printf("%s awake\n", sem->slptsk_head->name);
  }
  else
    panic("unexpected error!");
}

static void kmt_sem_wait(sem_t *sem) {
  // TRACE_ENTRY;
  sem->value--;
  if (sem->value < 0)
    sleep(sem);
}

static void kmt_sem_signal(sem_t *sem) {
  sem->value++;
  if (sem->value <= 0)
    wakeup(sem);
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


static int ncli[MAX_CPU], intena[MAX_CPU];

static void pushcli() {
  int efl = get_efl();
  cli();
  if (ncli[_cpu()] == 0)
    intena[_cpu()] = efl & FL_IF;
  ncli[_cpu()] += 1;
}

static void popcli() {
  if (get_efl() & FL_IF)
    panic("popcli - interruptible");
  if (--ncli[_cpu()] < 0)
    panic("popcli");
  if (ncli[_cpu()] == 0 && intena[_cpu()])
    sti();
}

static int holding (spinlock_t *lk) {
  int r;
  pushcli();
  r = lk->locked && lk->cpu == _cpu();
  popcli();
  return r;
}

