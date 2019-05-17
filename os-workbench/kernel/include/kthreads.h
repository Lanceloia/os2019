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

/* data-structure
 * tasks_list_head, current_tasks
 */

static spinlock_t tasks_list_mutex;
static task_t *tasks_list_head = NULL;

static spinlock_t current_tasks_mutex;
static task_t *current_tasks[MAX_CPU];
#define current (current_tasks[_cpu()])

/* callback-functions
 * context_save(), context_switch()
 */

static _Context *kmt_context_save(_Event ev, _Context *ctx) {
  if (current)
    current->ctx = *ctx;
  return NULL;
}

static _Context *kmt_context_switch(_Event ev, _Context *ctx) {
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

/* tasks-manage
 * push_back(), remove()
 */

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

/* spinlock-manage
 * pushcli(), popcli(), holding()
 */

static volatile int ncli[MAX_CPU] = {} , intena[MAX_CPU] = {};

static void pushcli() {
  cli();
  if (ncli[_cpu()] == 0)
    intena[_cpu()] = get_efl() & FL_IF;
  ncli[_cpu()] ++;
}

static void popcli() {
  ncli[_cpu()] --;
  if (ncli[_cpu()] == 0 && intena[_cpu()])
    sti();
}

static int holding(spinlock_t *lk) {
  return lk->locked && lk->cpu == _cpu();
}

/* task-null
 * null()
 */

static task_t task_null;

static void null(void *arg) {while(1);}

#endif
