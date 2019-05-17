#ifndef __KTHREADS_H__
#define __KTHREADS_H__

 void kmt_init();
 int kmt_create(task_t *, const char *, void (*)(void *), void *);
 void kmt_teardown(task_t *);
 void kmt_spin_init(spinlock_t *, const char *);
 void kmt_spin_lock(spinlock_t *);
 void kmt_spin_unlock(spinlock_t *);
 void kmt_sem_init(sem_t *, const char *, int);
 void kmt_sem_wait(sem_t *);
 void kmt_sem_signal(sem_t *);

/* data-structure
 * tasks_list_head, current_tasks
 */

 spinlock_t tasks_list_mutex;
 task_t *tasks_list_head = NULL;

 spinlock_t current_tasks_mutex;
 task_t *current_tasks[MAX_CPU];
#define current (current_tasks[_cpu()])

/* tasks-manage
 * push_back(), remove()
 */

void tasks_push_back(task_t *x) {
  x->next = tasks_list_head;
  tasks_list_head = x;
}

 void tasks_remove(task_t *x) {
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

 volatile int ncli[MAX_CPU] = {} , intena[MAX_CPU] = {};

 void pushcli() {
  cli();
  if (ncli[_cpu()] == 0)
    intena[_cpu()] = get_efl() & FL_IF;
  ncli[_cpu()] ++;
}

 void popcli() {
  ncli[_cpu()] --;
  if (ncli[_cpu()] == 0 && intena[_cpu()])
    sti();
}

 int holding(spinlock_t *lk) {
  return lk->locked && lk->cpu == _cpu();
}

/* task-null
 * null()
 */

 task_t task_null;

 void null(void *arg) {while(1);}

#endif
