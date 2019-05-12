#include <common.h>
#include <klib.h>

#define TRACEME

#ifdef TRACEME
  #define TRACE_ENTRY printf("[trace] %s:entry\n", __func__)
  #define TRACE_EXIT printf("[trace] %s:exit\n", __func__)
#else
  #define TRACE_ENTRY ((void)0)
  #define TRACE_EXIT ((void)0)
#endif

static _Context *kmt_context_save(_Event ev, _Context *context) { return NULL; }

static _Context *kmt_context_switch(_Event ev, _Context *context) { return NULL; }

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
  task->stack = pmm->alloc(STACK_SIZE);
  task->entry = entry;
  task->arg = arg;
  task->state = RUNNABLE;
  printf("[log] created task [%s]\n", task->name);
  TRACE_EXIT;
  return 0; 
}

static void kmt_teardown(task_t *task) {
  TRACE_ENTRY;
  TRACE_EXIT;
}

/* KMT spin-lock
 * 
 * kmt_spin_init: initialize the new lock saved at [lk], name it with [name]
 * kmt_spin_lock: try to lock the [lk]
 * kmt_spin_unlock: try to unlock the [lk]
 */

static void kmt_spin_init(spinlock_t *lk, const char *name) {
  // TRACE_ENTRY;
  strcpy(lk->name, name);
  printf("[log] created spinlock [%s]\n", lk->name);
  // TRACE_EXIT;
}

static void kmt_spin_lock(spinlock_t *lk) {
  // TRACE_ENTRY;
  // TRACE_EXIT;
}

static void kmt_spin_unlock(spinlock_t *lk) {
  // TRACE_ENTRY;
  // TRACE_EXIT;
}

/* KMT semaphore
 *
 * I'm lazy
 */

static void kmt_sem_init(sem_t *sem, const char *name, int value) {
  // TRACE_ENTRY;
  strcpy(sem->name, name);
  printf("[log] created semaphore [%s]\n", sem->name);
  // TRACE_EXIT;
}

static void kmt_sem_wait(sem_t *sem) {
  // TRACE_ENTRY;
  // TRACE_EXIT;
}

static void kmt_sem_signal(sem_t *sem) {
  // TRACE_ENTRY;
  // TRACE_EXIT;
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
