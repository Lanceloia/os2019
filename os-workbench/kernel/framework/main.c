#include <kernel.h>
#include <klib.h>
#include "../include/common.h"

sem_t empty, full, mutex;
const int maxk = 9;
int cnt;

static void producer(void *arg) {
  while(1) {
    kmt->sem_wait(&empty);
    kmt->sem_wait(&mutex);
    cnt ++;
    printf("%d%c ", cnt, (_cpu() == 0) ? 'a' : 'b');
    kmt->sem_signal(&mutex);
    kmt->sem_signal(&full);
  }
}

static void consumer(void *arg) {
  while(1) {
    kmt->sem_wait(&full);
    kmt->sem_wait(&mutex);
    cnt --;
    kmt->sem_signal(&mutex);
    kmt->sem_signal(&empty);
  }
}

static void create_threads() {
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
