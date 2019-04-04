#include <common.h>
#include <klib.h>

//#define DEBUG

static void os_init() {
  pmm->init();
}

static void hello() {
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++) {
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
}

#ifdef DEBUG 
static void test() {
  hello();
#define TEST_SIZE 128
#define FOR(var, start, end) \
  for(int var = start; var < end; i++)
#define ALLOC(integer) \
  do { \
    FOR(i, 0, TEST_SIZE){ \
      int tmp = (rand() % 128) * 1024; \
      if(arr[i] == NULL) { \
        arr[i] = pmm->alloc(tmp); \
        arr[i][0] = i + integer * 1000; \
      } \
      assert(arr[i] != NULL); \
    } \
  }while(0) 
#define FREE() \
  do{ \
    FOR(i, 0, TEST_SIZE){ \
      assert(arr[i]); \
      if(rand() % 2){ \
        pmm->free(arr[i]); \
        arr[i] = NULL; \
      } \
    } \
  }while(0)
#define SHOW() \
  do { \
    printf("\nCompare: \n"); \
    FOR(i, 0, TEST_SIZE){ \
      if(arr[i]) \
      printf("arr[%3d][0]=%4d \t", i, arr[i][0]); \
      else \
      printf("arr[%3d][0]=miss \t", i); \
      if(i % 3 == 2) \
      printf("\n"); \
    } \
    printf("\n"); \
  }while(0)
#define ALLFREE() \
  do{ \
    FOR(i, 0, TEST_SIZE){ \
      if(arr[i]){ \
        pmm->free(arr[i]); \
        arr[i] = NULL; \
      } \
    } \
  }while(0)

  extern void extern_free_print();
  int *arr[TEST_SIZE]={};
  extern_free_print(0);
  ALLOC(0);
  //SHOW();
  //extern_free_print();
  FREE();
  //SHOW();
  //extern_free_print();
  ALLOC(1);
  //SHOW();
  extern_free_print(1);
  FREE();
  //SHOW();
  extern_free_print(2);
  ALLOC(2);
  //SHOW();
  //extern_free_print();
  FREE();
  ALLOC(3);
  //SHOW();
  ALLFREE();
  //SHOW();
  extern_free_print(3);

#undef SHOW
#undef FREE
#undef ALLOC
#undef FOR
#undef TEST_SIZE
}
#endif

static void os_run() {
  hello();
#ifdef DEBUG
  test();
#endif
  _intr_write(1);
  while (1) {
    _yield();
  }
}

static _Context *os_trap(_Event ev, _Context *context) {
  return context;
}

static void os_on_irq(int seq, int event, handler_t handler) {
}

MODULE_DEF(os) {
  .init   = os_init,
    .run    = os_run,
    .trap   = os_trap,
    .on_irq = os_on_irq,
};
