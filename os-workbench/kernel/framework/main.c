#define _LANCELOIA_DEBUG_

#ifdef _LANCELOIA_DEBUG_

#include <kernel.h>
#include <klib.h>
#include "../include/common.h"

sem_t empty, full, mutex;
const int maxk = 8;
int cnt = 0;

static void producer(void *arg) {
  while(1) {
    kmt->sem_wait(&empty);
    kmt->sem_wait(&mutex);
    cnt ++;
    printf("%d+%c\t", cnt, _cpu()+'a');
    kmt->sem_signal(&mutex);
    kmt->sem_signal(&full);
  }
}

static void consumer(void *arg) {
  while(1) {
    kmt->sem_wait(&full);
    kmt->sem_wait(&mutex);
    cnt --;
    printf("%d-%c\t", cnt, _cpu()+'a');
    kmt->sem_signal(&mutex);
    kmt->sem_signal(&empty);
  }
}

static void create_threads() {
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-1: producer", producer, NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-2: producer", producer, NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-3: consumer", consumer, NULL);
  // kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-4: consumer", consumer, NULL);
  task_t *temp = pmm->alloc(sizeof(task_t));
  kmt->create(temp, "test-thread-4: consumer", consumer, NULL);
  //kmt->teardown(temp);
  kmt->sem_init(&empty, "buffer-empty", maxk);
  kmt->sem_init(&full, "buffer-full", 0);
  kmt->sem_init(&mutex, "mutex", 1);
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

#else

#include <kernel.h>
#include <klib.h>

int main() {
  _ioe_init();
  _cte_init(os->trap);

  // call sequential init code
  os->init();
  _mpe_init(os->run); // all cores call os->run()
  return 1;
}

#endif