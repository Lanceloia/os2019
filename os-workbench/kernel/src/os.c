#include <common.h>
#include <klib.h>

//#define DEBUG

static void os_init() {
  pmm->init();
  kmt->init();
  _vme_init(pmm->alloc, pmm->free);
  dev->init();
}

static void hello() {
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++) {
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
}

static void os_run() {
  hello();
/*
#ifdef DEBUG
  void test();
  test();
#endif
*/
  // if(_cpu() != 0)
  //  while(1);
  _intr_write(1);
  while (1) {
    _yield();
  }
}

struct item {
  int seq;
  int event;
  handler_t handler;
};

#define MAX_HANDLER 128

struct {
 int size;
 struct item items[MAX_HANDLER];
}ITEM;

void ITEM_bubble_sort() {
  for (int i = 0; i < ITEM.size; i++)
    for (int j = i + 1; j < ITEM.size; j++){
      if (ITEM.items[i].seq > ITEM.items[j].seq) {
        int tmp_seq = ITEM.items[i].seq;
        ITEM.items[i].seq = ITEM.items[j].seq, ITEM.items[j].seq = tmp_seq;
        int tmp_event = ITEM.items[i].event;
        ITEM.items[i].event = ITEM.items[j].event, ITEM.items[j].event = tmp_event;
        handler_t tmp_handler = ITEM.items[i].handler;
        ITEM.items[i].handler = ITEM.items[j].handler, ITEM.items[j].handler = tmp_handler;
      }
    }
}

static _Context *os_trap(_Event ev, _Context *context) {
  // TRACE_ENTRY;
  kmt_spin_lock(&current_tasks_mutex);
  _Context *ret = NULL;
  for (int i = 0; i < ITEM.size; i++) {
    if (ITEM.items[i].event == _EVENT_NULL || ITEM.items[i].event == ev.event) {
      _Context *next = ITEM.items[i].handler(ev, context);
      if (next) ret = next;
    }
  }
  kmt_spin_unlock(&current_tasks_mutex);
  return ret;
}

static void os_on_irq(int seq, int event, handler_t handler) {
  ITEM.items[ITEM.size].seq = seq;
  ITEM.items[ITEM.size].event = event;
  ITEM.items[ITEM.size].handler = handler;
  ITEM.size ++;
  ITEM_bubble_sort();
  printf("%s: seq:%d, event:%d\n", __func__, seq, event);
}

MODULE_DEF(os) {
  .init   = os_init,
    .run    = os_run,
    .trap   = os_trap,
    .on_irq = os_on_irq,
};


/*
#ifdef DEBUG 
static void test() {
  hello();
#define TEST_SIZE 800
#define LITTLE_BLOCK_SIZE 16
#define LARGE_BLOCK_SIZE 32
#define FOR(var, start, end) \
  for(int var = start; var < end; i++)
#define ALLOC(integer) \
  do { \
    FOR(i, 0, TEST_SIZE){ \
      int tmp = (rand() % 6 < 5) ? \
        ((rand() % LITTLE_BLOCK_SIZE) * 1024): \
        (((rand() % LARGE_BLOCK_SIZE) + LARGE_BLOCK_SIZE) * 1024); \
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
  //extern_free_print(0);
  ALLOC(0);
  //SHOW();
  //extern_free_print();
  FREE();
  //SHOW();
  //extern_free_print();
  ALLOC(1);
  //SHOW();
  //extern_free_print(1);
  FREE();
  //SHOW();
  //extern_free_print(2);
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
*/

