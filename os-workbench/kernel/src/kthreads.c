#include <common.h>
#include <klib.h>
#include <kthreads.h>

#define STACK_SIZE 4096

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

static int tasks_list_size = 0;
static task_t *tasks_list_head = NULL;
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
    if (!current || current->next == NULL)
      current = tasks_list_head;
    else
      current = current->next;
    // printf("%d ", _cpu());
  } while (!(current->state == STARTED || current->state == RUNNABLE));
 
  current->state = RUNNING;
  return &current->ctx;
}

static void tasks_insert(task_t *x) {
  kmt_spin_lock(&tasks_list_mutex);
  x->next = tasks_list_head;
  tasks_list_head = x;
  tasks_list_size ++;
  kmt_spin_unlock(&tasks_list_mutex);
}

static void tasks_remove(task_t *x) {
  kmt_spin_lock(&tasks_list_mutex);
  if (tasks_list_head == NULL)
    panic("\nERROR: tasks_remove error 0!\n");

  if (tasks_list_head->next == NULL) {
    if (tasks_list_head != x)
      panic("\nERROR: tasks_remove error 1!\n");
    tasks_list_head = NULL;
  }
  else {
    if (tasks_list_head == x)
      tasks_list_head = tasks_list_head->next;
    else{
      task_t *p = tasks_list_head;
      while(p->next != NULL && p->next != x) {p = p->next;}
      if(p->next == NULL)
        panic("\nERROR: tasks_remove error 2!\n");
      else
        p->next = p->next->next;
    }
  }
  tasks_list_size --;
  kmt_spin_unlock(&tasks_list_mutex);
}

static void kmt_init() {
  os->on_irq(INT32_MIN, _EVENT_NULL, kmt_context_save);
  os->on_irq(INT32_MAX, _EVENT_NULL, kmt_context_switch);
  kmt_spin_init(&tasks_list_mutex, "tasks-list-mutex");
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
  strcpy(task->name, name);
  task->stk.start = pmm->alloc(STACK_SIZE);
  task->stk.end = task->stk.start + STACK_SIZE;
  task->ctx = *(_kcontext(task->stk, entry, arg));
  task->state = STARTED;
  task->next = NULL;
  task->next2 = NULL;
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
  while(_atomic_xchg(&lk->locked, LOCKED));
  lk->cpu = _cpu();

  __sync_synchronize();
}

static void kmt_spin_unlock(spinlock_t *lk) {
  if (!holding(lk)) {
    printf("\nERROR: spin_unlock error! lk->name: %s\n", 
      lk->name);

    _halt(1);
  }

  lk->cpu = NONE_CPU;
  _atomic_xchg(&(lk->locked), UNLOCKED);
  popcli();
  for(volatile int i = 0; i < 10000; i++);
  __sync_synchronize();
}

/* semaphore
 * sem_init(), sem_wait(), sem_signal()
 */

static void kmt_sem_init(sem_t *sem, const char *name, int value) {
  strcpy(sem->name, name);
  sem->value = value;
  kmt_spin_init(&sem->lk, name);
  sem->head = NULL;
  printf("[log] created semaphore [%s]\n", sem->name);
}

static void sleep (sem_t *sem) {
  kmt_spin_lock(&current_tasks_mutex);
  current->state = YIELD;
  // tasks_remove(current);
  current->next2 = sem->head;
  sem->head = current;
  kmt_spin_unlock(&current_tasks_mutex);
  kmt_spin_unlock(&sem->lk);
  _yield();
}

static void wakeup (sem_t *sem) {
  if (sem->head == NULL) {
    printf("\nERROR: wakeup error! sem->name: %s, sem->value: %d\n", 
      sem->name, sem->value);
    _halt(1);
  }
  kmt_spin_lock(&current_tasks_mutex);
  assert(sem->head->state == YIELD);
  sem->head->state = RUNNABLE;
  sem->head = sem->head->next2;
  // tasks_insert(task);
  kmt_spin_unlock(&current_tasks_mutex);
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


