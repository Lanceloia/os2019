#include <kernel.h>
#include <klib.h>
#include "../include/common.h"

static void producer(void *arg) {
  while(1) {
    for(volatile int i = 0; i < 100000; i++);
    _putc('(');
  }
}

static void consumer(void *arg) {
  while(1) {
    for(volatile int i = 0; i < 100000; i++);
    _putc(')');
  }
}

static void create_threads() {
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-1: producer", producer, "xxx");
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-2: consumer", consumer, "yyy");
}

int main() {
  _ioe_init();
  _cte_init(os->trap);

  // call sequential init code
  os->init();
  create_threads();
  _mpe_init(os->run); // all cores call os->run()
  return 1;
}
