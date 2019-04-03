/* lock.h
 * copyright: Lanceloia
 * date: 2019/4/3
 */

struct spinlock_t{
  int locked;
  int cpu;
};
typedef struct spinlock_t spinlock_t;

void lock();

void unlock();

int atomic_xchg(int *, int);

void spin_lock();

void spin_unlock();
