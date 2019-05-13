#ifndef __KTHREADS_H__
#define __KTHREADS_H__

#include <common.h>

#define TASK_POOL_SIZE 128

static task_t *tasks_head = NULL;

static void tasks_push_back(task_t *x) {
  x->next = tasks_head;
  tasks_head = x;
}

static void tasks_remove(task_t *x) {
  if (tasks_head == NULL) 
    panic("tasks_remove failed 0");
  else if (tasks_head->next == NULL) {
    if (tasks_head != x)
      panic("tasks_remove failed 1");
    else
      tasks_head = NULL;
  }
  else {
    if (tasks_head == x)
      tasks_head = tasks_head->next;
    else{
      task_t *p = tasks_head, *q = tasks_head->next;
      while(q != NULL && q != x) {p = p->next, q = q->next;}
      if (q == NULL)
        panic("task_remove failed 2");
      else
        p->next = q->next;
    }
  }
}

#endif
