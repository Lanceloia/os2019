#include<common.h>
#include<klib.h>
#include<lock.h>

static int atomic_xchg(volatile int *addr, int newval){
  int result;
  asm volatile ("lock xchg %0, %1":
    "+m"(*addr), "=a"(result) : "1"(newval) : "cc");
  return result;
}

/*
static void lock(){
  _intr_write(0);
}

static void unlock(){
  _intr_write(1);
}
*/

void mutex_lock(lock_t *lk){
  while(atomic_xchg(&lk->locked, 1)) ;
}

void mutex_unlock(lock_t *lk){
  atomic_xchg(&lk->locked, 0);
}
