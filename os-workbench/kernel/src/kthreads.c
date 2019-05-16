#include <common.h>
#include <klib.h>
#include <kthreads.h>


static void kmt_init() {
  os->on_irq(INT32_MIN, _EVENT_NULL, kmt_context_save);
  os->on_irq(INT32_MAX, _EVENT_NULL, kmt_context_switch);
  kmt->spin_init(&tasks_mutex, "tasks-mutex");
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

/* spinlock
 * spin_init(), spin_lock(), spin_unlock()
 */

static void kmt_spin_init(spinlock_t *lk, const char *name) {
  strcpy(lk->name, name);
  lk->locked = UNLOCKED;
  printf("[log] created spinlock [%s]\n", lk->name);
}

static void kmt_spin_lock(spinlock_t *lk) {
  pushcli();
  while(_atomic_xchg(&lk->locked, LOCKED));
}

static void kmt_spin_unlock(spinlock_t *lk) {
  _atomic_xchg(&lk->locked, UNLOCKED);
  popcli();
}

/* semaphore
 * sem_init(), sem_wait(), sem_signal()
 */

static void kmt_sem_init(sem_t *sem, const char *name, int value) {
  strcpy(sem->name, name);
  sem->value = value;
  kmt_spin_init(&sem->lk, name);
  sem->slptsk_head = NULL;
  printf("[log] created semaphore [%s]\n", sem->name);
}

static void sleep (sem_t *sem) {
  kmt->spin_lock(&tasks_mutex);
  tasks_remove(current);

  current->next = sem->slptsk_head;
  sem->slptsk_head = current;
  kmt->spin_unlock(&tasks_mutex);
  _yield();
}

static void wakeup (sem_t *sem) {
  assert(sem->slptsk_head != NULL);
  kmt->spin_lock(&tasks_mutex);
  task_t *task = sem->slptsk_head;
  sem->slptsk_head = sem->slptsk_head->next;
  task->state = RUNNABLE;
  tasks_push_back(task);
  kmt->spin_unlock(&tasks_mutex);

}

static void kmt_sem_wait(sem_t *sem) {
  kmt_spin_lock(&sem->lk);
  sem->value--;
  kmt_spin_unlock(&sem->lk);
  if (sem->value < 0)
    sleep(sem);
}

static void kmt_sem_signal(sem_t *sem) {
  kmt_spin_lock(&sem->lk);
  sem->value++;
  kmt_spin_unlock(&sem->lk);
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


