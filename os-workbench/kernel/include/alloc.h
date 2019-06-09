#ifndef __ALLOC_H__
#define __ALLOC_H__

struct mem_block {
  int id, state;
  size_t size;
  uintptr_t begin, end;
  struct mem_block *prev;
  struct mem_block *next;
};

typedef struct mem_block mem_block;

struct block_list {
  struct mem_block *head;
  struct mem_block *tail;
  size_t list_size;
};

typedef struct block_list block_list;

#endif