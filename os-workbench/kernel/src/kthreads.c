#include <common.h>
#include <klib.h>
#include <kthreads.h>

static _Context *kmt_context_save(_Event ev, _Context *ctx) {
  if (current)
    current->ctx = *ctx;
  return NULL;
}

static _Context *kmt_context_switch(_Event ev, _Context *ctx) {
  if(current->state == RUNNING)
    current->state = RUNNABLE;

  do {
    // for(volatile int i = 0; i < 10000; i ++);
    if (!current || current->next == NULL)
      current = tasks_list_head;
    else
      current = current->next;
  } while (!(current->state == STARTED || current->state == RUNNABLE));
 
  current->state = RUNNING;
  return &current->ctx;
}

static void tasks_push_back(task_t *x) {
  x->next = tasks_list_head;
  tasks_list_head = x;
}

static void tasks_remove(task_t *x) {
  assert(tasks_list_head != NULL); 
  if (tasks_list_head->next == NULL) {
    assert(tasks_list_head == x);
    tasks_list_head = NULL;
  }
  else {
    if (tasks_list_head == x)
      tasks_list_head = tasks_list_head->next;
    else{
      task_t *p = tasks_list_head;
      while(p->next != NULL && p->next != x) {p = p->next;}
      if(p->next != NULL)
        p->next = p->next->next;
      else
        {printf("WARNING: remove error!\n"); _halt(1);}
    }
  }
}

void kmt_init() {
  os->on_irq(INT32_MIN, _EVENT_NULL, kmt_context_save);
  os->on_irq(INT32_MAX, _EVENT_NULL, kmt_context_switch);
  kmt_spin_init(&tasks_list_mutex, "tasks-list-mutex");
  kmt_spin_init(&current_tasks_mutex, "current-tasks-mutex");
  kmt_create(&task_null, "task-null", null, NULL);
}

/* tasks
 * create(), teardown()
 */

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  strcpy(task->name, name);
  task->stk.start = pmm->alloc(STACK_SIZE);
  task->stk.end = task->stk.start + STACK_SIZE;
  task->ctx = *(_kcontext(task->stk, entry, arg));
  task->state = STARTED;
  
  kmt_spin_lock(&tasks_list_mutex);
  tasks_push_back(task);
  kmt_spin_unlock(&tasks_list_mutex);
  printf("[task] created [%s]\n", task->name);
  return 0; 
}

void kmt_teardown(task_t *task) {
  TRACE_ENTRY;
  assert(0);
  tasks_remove(current);
  TRACE_EXIT;
}

/* spinlock
 * spin_init(), spin_lock(), spin_unlock()
 */

void kmt_spin_init(spinlock_t *lk, const char *name) {
  strcpy(lk->name, name);
  lk->locked = UNLOCKED;
  lk->cpu = -1;
  printf("[log] created spinlock [%s]\n", lk->name);
}

void kmt_spin_lock(spinlock_t *lk) {
  if (holding(lk)) {printf("%s, locked\n", lk->name); _halt(1);}
  pushcli();
  while(_atomic_xchg(&lk->locked, LOCKED));
  lk->cpu = _cpu();
  __sync_synchronize();
}

void kmt_spin_unlock(spinlock_t *lk) {
  if (!holding(lk)) {printf("%s, unlocked\n", lk->name); _halt(1);}
  lk->cpu = -1;
  _atomic_xchg(&(lk->locked), UNLOCKED);
  popcli();
  __sync_synchronize();
}

/* semaphore
 * sem_init(), sem_wait(), sem_signal()
 */

void kmt_sem_init(sem_t *sem, const char *name, int value) {
  strcpy(sem->name, name);
  sem->value = value;
  kmt_spin_init(&sem->lk, name);
  sem->head = NULL;
  printf("[log] created semaphore [%s]\n", sem->name);
}

void sleep (sem_t *sem) {
  kmt_spin_lock(&current_tasks_mutex);
  current->state = YIELD;
  kmt_spin_lock(&tasks_list_mutex);
  tasks_remove(current);
  kmt_spin_unlock(&tasks_list_mutex);
  current->next = sem->head;
  sem->head = current;
  kmt_spin_unlock(&current_tasks_mutex);
  kmt_spin_unlock(&sem->lk);
  _yield();
}

void wakeup (sem_t *sem) {
  if (sem->head == NULL) {printf("WARNING: wakeup error! sem->value: %d\n", sem->value); _halt(1);}
  task_t *task = sem->head;
  sem->head = sem->head->next;
  task->state = RUNNABLE;
  kmt_spin_lock(&tasks_list_mutex);
  tasks_push_back(task);
  kmt_spin_unlock(&tasks_list_mutex);
  kmt_spin_unlock(&sem->lk);
}

void kmt_sem_wait(sem_t *sem) {
  kmt_spin_lock(&sem->lk);
  sem->value--;
  //for(volatile int i = 0; i < 5000; i++);
  if (sem->value < 0)
    sleep(sem);
  else
    kmt_spin_unlock(&sem->lk);
}

void kmt_sem_signal(sem_t *sem) {
  kmt_spin_lock(&sem->lk);
  sem->value++;
  //for(volatile int i = 0; i < 5000; i++);
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


