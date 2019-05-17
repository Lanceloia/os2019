#ifndef __KTHREADS_H__
#define __KTHREADS_H__

/* data-structure
 * tasks_mutex, current_task
 */

static spinlock_t tasks_mutex;

static task_t *current_task[MAX_CPU];

#define current (current_task[_cpu()])

static task_t *tasks_head = NULL;

/* callback-functions
 * context_save(), context_switch()
 */

static void kmt_spin_lock(spinlock_t *);
static void kmt_spin_unlock(spinlock_t *);

static _Context *kmt_context_save(_Event ev, _Context *ctx) {
  kmt_spin_lock(&tasks_mutex);
  if (current)
    current->ctx = *ctx;
  kmt_spin_unlock(&tasks_mutex);
  return NULL;
}

static _Context *kmt_context_switch(_Event ev, _Context *ctx) {
  kmt_spin_lock(&tasks_mutex);
  current->state = RUNNABLE;

  do {
    // for(volatile int i = 0; i < 10000; i ++);
    if (!current || current->next == NULL)
      current = tasks_head;
    else
      current = current->next;
    //printf("%d", 1);
  } while (!(current->state == STARTED || current->state == RUNNABLE));
 
  current->state = RUNNING;
  kmt_spin_unlock(&tasks_mutex);
  return &current->ctx;
}

/* tasks-manage
 * push_back(), remove()
 */

static void tasks_push_back(task_t *x) {
  x->next = tasks_head;
  tasks_head = x;
}

static void tasks_remove(task_t *x) {
  assert(tasks_head != NULL); 
  if (tasks_head->next == NULL) {
    assert(tasks_head == x);
    tasks_head = NULL;
  }
  else {
    if (tasks_head == x)
      tasks_head = tasks_head->next;
    else{
      task_t *p = tasks_head;
      while(p->next != NULL && p->next != x) {p = p->next;}
      // assert(p->next != NULL);
      // p->next = p->next->next;
      if(p->next != NULL)
        p->next = p->next->next;
      else
        printf("WARNING: remove error!\n");
    }
  }
}

/* spinlock-manage
 * pushcli(), popcli()
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

/* task-null
 * null()
 */

static task_t task_null;

static void null(void *arg) {while(1);}

#endif
