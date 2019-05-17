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

#endif