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
    //printf("%d+%c\t", cnt, _cpu()+'a');
    kmt->sem_signal(&mutex);
    kmt->sem_signal(&full);
  }
}

static void consumer(void *arg) {
  while(1) {
    kmt->sem_wait(&full);
    kmt->sem_wait(&mutex);
    cnt --;
    //printf("%d-%c\t", cnt, _cpu()+'a');
    kmt->sem_signal(&mutex);
    kmt->sem_signal(&empty);
  }
}

static void motherfucker(void *arg) {
  task_t *producer_task = NULL, *consumer_task = NULL;
  while (1) {
    if (!producer_task && rand() % 2) {
      producer_task = pmm->alloc(sizeof(task_t));
      kmt->create(producer_task, "test-thread-3: producer", producer, NULL);
    }
    
    if (!consumer_task && rand() % 2) {
      consumer_task = pmm->alloc(sizeof(task_t));
      kmt->create(consumer_task, "test-thread-4: consumer", consumer, NULL);
    }

    if (producer_task && rand() % 2) {
      kmt->teardown(producer_task);
      pmm->free(producer_task);
      producer_task = NULL;
    }

    if (consumer_task && rand() % 2) {
      kmt->teardown(consumer_task);
      pmm->free(consumer_task);
      consumer_task = NULL;
    }
  }
}

static void create_threads() {
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-0: motherfucker", motherfucker, NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-1: producer", producer, NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-2: consumer", consumer, NULL);
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