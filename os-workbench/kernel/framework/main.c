#include <kernel.h>
#include <klib.h>
#include "../include/common.h"

static void producer(void *arg) {
  assert(0);
}

static void consumer(void *arg) {
  assert(0);
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
