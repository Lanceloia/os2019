#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "co.h"

#define MAX_CO 128
#define STK_SIZE 4096

#if defined(__i386__)
#define SP "%%esp"
#elif defined(__x86_64__)
#define SP "%%rsp"
#endif

#define END 0
#define RUNNING 1

char *__stk, *__stk_bk;
func_t __func;
void *__arg;

struct co {
  int id, state;
  jmp_buf buf;
  char stk[STK_SIZE];
  char *stk_bk;
}__attribute__((aligned(16)));

struct co *current, *next, co_rtns[MAX_CO];
int co_cnt, act_cnt;

void co_init() {
  co_cnt = 1;
  act_cnt = 1;
  
  current = &co_rtns[co_cnt - 1];
  current->id = co_cnt - 1;
  current->state = RUNNING;
  srand(time(NULL));
}

struct co* co_start(const char *name, func_t func, void *arg) {
  co_cnt ++;
  act_cnt ++; // It's confused that can't move this statement to line 69.
  __func = func;
  __arg = arg;
 
  int val = setjmp(co_rtns[0].buf);
  if(val == 0){
    current = &co_rtns[co_cnt-1];
    current->id = co_cnt-1;
   
    __stk = current->stk + sizeof(current->stk);
    //switch stk_frame
    asm volatile("mov " SP ", %0; mov %1, " SP :
        "=g"(__stk_bk) :
        "g"(__stk));
    current->stk_bk = __stk_bk;
    current->state = RUNNING;
    __func(__arg);
    current->state = END;
    
    act_cnt --; // Crazy 
    longjmp(co_rtns[0].buf, co_rtns[0].id);
  }
  return &co_rtns[co_cnt-1];
}

int select_co(){
  int num = rand() % act_cnt;
  for(int i = 0; i < co_cnt; i ++)
    if(co_rtns[i].state == RUNNING){
      if(num == 0)
        return i;
      else
        num --;
    }
  return -1;
}

#define SWICTH_AND_JMUP() \
  do{ \
    int m = select_co(); \
    while(m == current->id) \
      m = select_co(); \
    next = &co_rtns[m]; \
    \
    current = next; \
    longjmp(current->buf, current->id); \
  }while(0) 

void co_yield() {
  int val = setjmp(current->buf);
  if(val == 0)
    SWICTH_AND_JMUP();
}

void co_wait(struct co *thd) {
  setjmp(current->buf); 
  if(thd->state != END)
    SWICTH_AND_JMUP();
}
