#include <common.h>
#include <os.h>

#define __TTY__

extern void echo_task(void *name);

static void os_init() {
  pmm->init();
  kmt->init();
  _vme_init(pmm->alloc, pmm->free);
  dev->init();

#ifdef __TTY__
  kmt->create(pmm->alloc(sizeof(task_t)), "print", echo_task, "tty1");
  kmt->create(pmm->alloc(sizeof(task_t)), "print", echo_task, "tty2");
  kmt->create(pmm->alloc(sizeof(task_t)), "print", echo_task, "tty3");
  kmt->create(pmm->alloc(sizeof(task_t)), "print", echo_task, "tty4");
#endif

}

static void hello() {
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++) {
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
}

static void os_run() {
  hello();
  _intr_write(1);
  while (1) {
    _yield();
  }
}

extern spinlock_t current_tasks_mutex;

static _Context *os_trap(_Event ev, _Context *context) {
  kmt->spin_lock(&current_tasks_mutex);
  _Context *ret = NULL;
  for (int i = 0; i < ITEM.size; i++) {
    if (ITEM.items[i].event == _EVENT_NULL || ITEM.items[i].event == ev.event) {
      _Context *next = ITEM.items[i].handler(ev, context);
      if (next) ret = next;
    }
  }
  kmt->spin_unlock(&current_tasks_mutex);
  return ret;
}

static void os_on_irq(int seq, int event, handler_t handler) {
  ITEM_push_back(seq, event, handler);
  ITEM_bubble_sort();
}

MODULE_DEF(os) {
  .init   = os_init,
    .run    = os_run,
    .trap   = os_trap,
    .on_irq = os_on_irq,
};
