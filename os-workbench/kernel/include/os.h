#ifndef __OS_H__
#define __OS_H__

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

static inline void ITEM_push_back(int seq, int event, handler_t handler) {
  ITEM.items[ITEM.size].seq = seq;
  ITEM.items[ITEM.size].event = event;
  ITEM.items[ITEM.size].handler = handler;
  ITEM.size ++;
}

static void ITEM_bubble_sort() {
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

#endif