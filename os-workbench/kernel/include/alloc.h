#ifndef __ALLOC_H__
#define __ALLOC_H__

struct mem_block {
  int id, state;
  size_t size;
  uintptr_t begin, end;
  struct mem_block *prev;
  struct mem_block *next;
};

typedef struct mem_block mem_block_t;

struct block_list {
  struct mem_block *head;
  struct mem_block *tail;
  size_t list_size;
};

typedef struct block_list block_list_t;


enum { UNUSED=0, UNALLOCATED=1, ALLOCATED=2 };
/* mem_block, three states
 * 1. UNUSED, totally new
 * 2. UNALLOCATED, belong to <list> free
 * 3. ALLOCATED, some processes used
 */


#endif