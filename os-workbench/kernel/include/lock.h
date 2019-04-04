/* lock.h
 * copyright: Lanceloia
 * date: 2019/4/4
 */

struct lock_t{
  int locked;
  int cpu;
};

typedef struct lock_t lock_t;

void mutex_lock(lock_t *lk);

void mutex_unlock(lock_t *lk);
