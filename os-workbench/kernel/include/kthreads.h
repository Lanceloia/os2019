#ifndef __KTHREADS_H__
#define __KTHREADS_H__

enum
{
  NIL = 0,
  STARTED = 1,
  RUNNABLE = 2,
  RUNNING = 3,
  YIELD = 4,
  KILLED = 5
};

/* data-structure
 * tasks_list_head, current_tasks
 */

naivelock_t tasks_mutex;
naivelock_t current_tasks_mutex;

/* spinlock-manage
 * pushcli(), popcli(), holding()
 */

naivelock_t cli_mutex = UNLOCKED;
volatile int ncli[MAX_CPU] = {}, intena[MAX_CPU] = {};

void pushcli()
{
  cli();
  if (ncli[_cpu()] == 0)
    intena[_cpu()] = get_efl() & FL_IF;
  naivelock_lock(cli_mutex);
  ncli[_cpu()]++;
  naivelock_unlock(cli_mutex);
}

void popcli()
{
  naivelock_lock(cli_mutex);
  ncli[_cpu()]--;
  naivelock_unlock(cli_mutex);
  if (ncli[_cpu()] == 0 && intena[_cpu()])
    sti();
}

int holding(spinlock_t *lk)
{
  return lk->locked && lk->cpu == _cpu();
}

task_t wait[MAX_CPU] = {};
void task_wait(void *arg)
{
  while (1)
    ;
}

#endif
