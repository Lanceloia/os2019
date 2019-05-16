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

static _Context *kmt_context_save(_Event ev, _Context *ctx) {
  if (current)
    current->ctx = *ctx;
  return NULL;
}

static _Context *kmt_context_switch(_Event ev, _Context *ctx) {
  if (!current || current->next == NULL)
    current = tasks_head;
  else
    current = current->next;
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
      assert(p->next != NULL);
      p->next = p->next->next;
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

#endif
