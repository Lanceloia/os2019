#include<common.h>
#include<klib.h>
#include<lock.h>

void lock(){
  _intr_write(0);
}

void unlock(){
  _intr_write(1);
}

int atomic_xchg(int *locked, int num){
  int temp = *locked;
  *locked = num;
  return temp;
}

void spin_lock(spinlock_t *lk){
  lock();
  while(atomic_xchg(&lk->locked, 1))
      _yield();//printf("death lock  ");
  unlock();
}

void spin_unlock(spinlock_t *lk){
  lock();
  atomic_xchg(&lk->locked, 0);
  unlock();
}
