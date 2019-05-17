/* mem_block, three states
 * 1. unused, totally new
 * 2. unallocated, belong to <list> free
 * 3. allocated, some processes used
 */

#include <common.h>
#include <klib.h>

#define POOL_SIZE (4*1024)

enum {
  UNUSED=0, UNALLOCATED=1, ALLOCATED=2
};

int memoplk;

//--- data structure ---//
struct mem_block {
  int id, state;
  size_t size;
  uintptr_t begin, end;
  struct mem_block *prev;
  struct mem_block *next;
}pool[POOL_SIZE], special[2];
typedef struct mem_block mem_block;

//--- data structure ---//
struct block_list {
  struct mem_block *head;
  struct mem_block *tail;
  size_t list_size;
}free;
typedef struct block_list block_list;

//--- pool helper functions ---//
/* function get_unused_block()
 * search for an unuesd block, for wich there is
 * state == UNUSED
 */
static int get_unused_block(){
  int ret = -1;
  for(int i = 0; i < POOL_SIZE; i++)
    if(pool[i].state == UNUSED){
      ret = i; break;
    }
  assert(ret >= 0);
  return ret;
}

/* function get_allocated_block()
 * search for the used block, for which there is
 * state == ALLOCATED and block->begin == begin
 */
static int get_allocated_block(uintptr_t begin){
  int ret = -1;
  for(int i = 0; i < POOL_SIZE; i++){
    if(pool[i].state == ALLOCATED && pool[i].begin == begin){
      ret = i; break;
    }
  }
  assert(ret >= 0);
  return ret;
}

//--- free member functions ---//
/* function free_init(): interface
 * initial the <list> free, keep a well-order when
 * start always help a lot
 */
static void free_init(uintptr_t begin, uintptr_t end){
  for(int i = 0; i < POOL_SIZE; i ++)
    pool[i].id = i, pool[i].state = UNUSED;
  // state == UNUSED means the node is totally new
  free.head = &special[0];
  free.tail = &special[1];
  free.head->prev = NULL, free.head->next = &pool[0];
  free.tail->prev = &pool[0], free.tail->next = NULL;
  pool[0].state = UNALLOCATED;
  pool[0].prev = free.head, pool[0].next = free.tail;
  pool[0].begin = begin, pool[0].end = end;
  pool[0].size = end - begin;
  free.list_size = 1;
}

/* function free_print()
 * print the <list> free, show each item's [id], [state],
 * [size], [begin], [next_id], notice that all the [state]
 * shouble [1]UNALLOCATED
 */
static void free_print(){
  mem_block *block = free.head->next;
  printf("---------------------------------------------\n");
  printf("  id   state      size      begin     next_id\n");
  printf("---------------------------------------------\n");
  while(block != free.tail){
    printf("%4d   %2d   %8dKB  %10x    %4d\n",
        block->id, block->state, block->size / 1024,
        block->begin, block->next->id);
    block = block->next;
  }
  printf("---------------------------------------------\n\n");
}

/* function extern_free_print(): interface
 * an extern interface of free_print()
 * print the current [cpu] and flag to make
 * checking log eaiser 
 */
void extern_free_print(int flag){
  spin_lock(&memoplk);
  printf("[CPU: %d Flag: %d]\n", _cpu(), flag);
  free_print();
  spin_unlock(&memoplk);
}

/* function free_check()
 * check the <list> free, if it failed, show the log
 * checkpoint 1: block->next->prev == block
 * checkpoint 2: block->prev->next == block
 * checkpoint 3: free.list_size == free.size()
 */
static void free_check(){
  int cnt = 0;
  mem_block *block = free.head->next;
  while(block != free.tail){
    cnt ++;
    assert(block->next->prev == block);
    assert(block->prev->next == block);
    block = block->next;
  }
  assert(cnt == free.list_size);
}

/* function free_cut()
 * cut block into 2 parts, the former should larger
 * than size, set the new part into unallocated, mean
 * the new part is belong to <list> free now.
 */
static void free_cut(mem_block *block, size_t size){
  int newid = get_unused_block();
  assert(block->state == UNALLOCATED);
  assert(pool[newid].id == newid);
  assert(pool[newid].state == UNUSED);
  mem_block *newblock = &pool[newid];
  newblock->prev = block, newblock->next = block->next;
  block->next->prev = newblock, block->next = newblock;

  newblock->begin = block->begin + size;
  newblock->end = block->end;
  newblock->size = block->size - size;
  block->begin = block->begin;
  block->end = block->begin + size;
  block->size = size;
  newblock->state = UNALLOCATED; 
  free.list_size ++;
}

/* function free_adjcent()
 * a helper function for free_merge(), judge two blocks
 * p and q whether is adjcent or not, if they are 
 * adjcent(p->end == q->begin) return true, otherwise
 * return false.
 */
static int free_adjcent(mem_block *p, mem_block *q){
  if(p == free.head || q == free.tail)
    return 0;
  assert(p != free.tail);
  assert(q != free.head);
  return p->end == q->begin;
}

/* function free_merge()
 * merge two adjcent block, the former and the latter. Two
 * nodes become one node. Actually, let former->end = 
 * latter->end, former->next = latter->next, latter->next->prev =
 * former, then remove the latter.
 */
static void free_merge(mem_block *former, mem_block *latter){
  assert(free_adjcent(former, latter));
  assert(former != free.head);
  assert(latter != free.tail);
  assert(former->state == UNALLOCATED);
  assert(latter->state == UNALLOCATED);

  // delete latter
  former->next = latter->next;
  latter->next->prev = former;
  latter->prev = NULL;
  latter->next = NULL;
  // resize the former
  assert(latter->end-former->begin
      ==former->size+latter->size);
  former->end = latter->end;
  former->size = former->end - former->begin;
  // set the latter UNUSED
  latter->state = UNUSED;
  free.list_size --;
  // the latter is freedom now
  free_check();
}

/* function free_cmp()
 * a helper function for free_insert(), compare block q
 * and block q, if p->begin < q->begin return true, 
 * otherwise return false
 */
static int free_cmp(mem_block *p, mem_block *q){
  if(p == free.head || q == free.tail)
    return 1;
  assert(p != free.tail);
  assert(q != free.head);
  assert(p->begin != q->begin);
  return p->begin < q->begin;
}

/* function free_insert(): interface
 * <list> free is a list with ascdent order, find out
 * the fitting place that block should be inserted,
 * then insert the block into <list> free.
 */
static void free_insert(mem_block *block){
  assert(block->state == ALLOCATED);
  mem_block *former = free.head;
  mem_block *latter = former->next;
  while(!(free_cmp(former, block) && free_cmp(block, latter))){
    assert(latter != free.tail);
    former = former->next;
    latter = latter->next;
  }
  assert(free_cmp(former, block) && free_cmp(block, latter));

  // link
  block->prev = former, block->next = latter;
  former->next = block, latter->prev = block;
  block->state = UNALLOCATED;
  free.list_size ++;
  // don't change their order
  if(free_adjcent(block, latter))
    free_merge(block, latter);
  if(free_adjcent(former, block))
    free_merge(former, block);
  free_check();
}

/* function free_delete(): interface
 * delete a node from <list> free  
 */
static void free_delete(mem_block *block){
  mem_block *former, *latter;
  assert(block != free.head);
  assert(block != free.tail);
  assert(block->state == UNALLOCATED);
  // block is belong to <list>
  former = block->prev;
  latter = block->next;
  former->next = latter;
  latter->prev = former;
  block->prev = NULL, block->next = NULL;
  block->state = ALLOCATED;
  free.list_size --;
  // block isn't belong to <list> now
}

/* function free_find(): interface
 * find a block whose size is greater than size and it
 * is unallocated. Notice that the members of <list> 
 * free all should be unallocated. 
 */
#define KB *1024
static mem_block *free_find(size_t size){
  size_t newsz = 1 KB;
  while(newsz < size) newsz <<= 1;
  mem_block *block = free.head->next;
  while((block != free.tail) && (block->size < newsz)){
    assert(block->state == UNALLOCATED);
    block = block->next;
  }
  assert(block != free.tail);
  assert(block->size >= newsz);
  if(block->size >= 2 * newsz)
    free_cut(block, newsz);
  free_delete(block);
  return block;
}
#undef KB

static uintptr_t pm_start, pm_end;

static void pmm_init() {
  spin_lock(&memoplk);

  pm_start = (uintptr_t)_heap.start;
  pm_end   = (uintptr_t)_heap.end;

  free_init(pm_start, pm_end);
  spin_unlock(&memoplk);
}


static void *kalloc(size_t size) {
  if(size == 0)
    return NULL;

  spin_lock(&memoplk);
  mem_block *block = free_find(size);
  assert(block != NULL);
  free_check();
  assert(block->begin != 0);
  spin_unlock(&memoplk);
  return (void *)block->begin;
}

static void kfree(void *ptr) {
  spin_lock(&memoplk);

  int idx = get_allocated_block((uintptr_t)ptr);
  free_insert(&pool[idx]);

  spin_unlock(&memoplk);
}

MODULE_DEF(pmm) {
  .init = pmm_init,
    .alloc = kalloc,
    .free = kfree,
};
