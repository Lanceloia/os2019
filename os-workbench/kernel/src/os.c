#include <common.h>
#include <os.h>

extern void shell_task(void *name);
extern void procfs_add(void *procname);

static void os_init() {
  pmm->init();
  kmt->init();
  dev->init();
  vfs->init();
  kmt->create(pmm->alloc(sizeof(task_t)), "print", shell_task, "tty1");
  kmt->create(pmm->alloc(sizeof(task_t)), "print", shell_task, "tty2");
  kmt->create(pmm->alloc(sizeof(task_t)), "print", shell_task, "tty3");
  kmt->create(pmm->alloc(sizeof(task_t)), "print", shell_task, "tty4");
}

static void hello() {
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++) {
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]);
  _putc('\n');
}

static void os_run() {
  hello();
  _intr_write(1);
  while (1) {
    _yield();
  }
}

extern naivelock_t current_tasks_mutex;

static _Context *os_trap(_Event ev, _Context *context) {
  naivelock_lock(&current_tasks_mutex);
  _Context *ret = NULL;
  for (int i = 0; i < ITEM.size; i++) {
    if (ITEM.items[i].event == _EVENT_NULL || ITEM.items[i].event == ev.event) {
      _Context *next = ITEM.items[i].handler(ev, context);
      if (next) ret = next;
    }
  }
  naivelock_unlock(&current_tasks_mutex);
  return ret;
}

static void os_on_irq(int seq, int event, handler_t handler) {
  ITEM_push_back(seq, event, handler);
  ITEM_bubble_sort();
}

MODULE_DEF(os){
    .init = os_init,
    .run = os_run,
    .trap = os_trap,
    .on_irq = os_on_irq,
};
