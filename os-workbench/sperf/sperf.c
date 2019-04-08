/* sperf.c
 * step 1: fork()
 * step 2: execve() strace -T
 * step 3: collect running time
 * step 4: analyze running time
 *
 * example usage 32: ./sperf-32 /bin/cat sperf.c
 * example usage 64: ./sperf-64 /bin/cat sperf.c
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define POOL_SIZE (1024)
#define syscall _syscall_list

//--- data structure ---//
struct list_node {
  int id;
  char name[1024];
  double time;
  struct list_node *prev;
  struct list_node *next;
}pool[POOL_SIZE], special[2];
typedef struct list_node list_node;

struct list {
  struct list_node *head;
  struct list_node *tail;
  size_t list_size;
  double total_time;
}syscall;
typedef struct list list;

//--- syscall member functions ---// 
/* function syscall_init(): interface
 * initial the <list> syscall, keep a well-order when
 * start always help a lot
 */
static void syscall_init() {
  syscall.head = &special[0];
  syscall.tail = &special[1];

  syscall.head->prev = NULL;
  syscall.head->next = syscall.tail;
  syscall.tail->prev = syscall.head;
  syscall.tail->next = NULL;

  syscall.list_size = 0;
  syscall.total_time = 0.0;
}

/* function syscall_print(): interface
 * print the <list> syscall, show each item's
 * [name], [percentage].
 */
static void syscall_print() {
  static int clear_screen_flag = 1;
  double remaining_time = syscall.total_time;
  int cnt = 0;
  if(clear_screen_flag) {
    fprintf(stdout, "\033[2J");
    clear_screen_flag = 0;
  }
  list_node *node = syscall.head->next;
  fprintf(stdout, "\033[0;0H");
  while(node != syscall.tail) {
    fprintf(stdout, "%-32s", node->name);
    fprintf(stdout, "%.1lf%%   \n",
        node->time * 100 / syscall.total_time);

    remaining_time -= node->time;
    cnt ++;
    
    if(cnt >= 16) {
      fprintf(stdout, "%-32s", "others");
      fprintf(stdout, "%.1lf%%   \n", 
          remaining_time * 100 / syscall.total_time);
      break;
    }
    
    node = node->next;
  }
}

/* function syscall_check()
 * check the <list> syscall
 * checkpoint 1: node->next->prev == node
 * checkpoint 2: node->prev->next == node
 * checkpoint 3: syscall.list_size == syscall.size()
 */
static void syscall_check() {
  int cnt = 0;
  list_node *node = syscall.head->next;
  while(node != syscall.tail) {
    cnt ++;
    assert(node->next->prev == node);
    assert(node->prev->next == node);
    node = node->next;
  }
  assert(cnt == syscall.list_size);
}

/* function syscall_cmp()
 * helper function, compare two node
 */
static int syscall_cmp(list_node *A, list_node *B){
  if(A == syscall.head || B == syscall.tail)
    return 1;
  assert(A != syscall.tail);
  assert(B != syscall.head);
  return A->time >= B->time;
}

/* function syscall_insert_do()
 * keep the <list> descending sort
 */
static void syscall_insert_do(list_node *node){
  list_node *temp = syscall.head;
  assert(node != syscall.tail);
  while(!(syscall_cmp(temp, node) && syscall_cmp(node, temp->next))) {
    temp = temp->next;
  }
  assert(temp != syscall.tail);

  node->prev = temp;
  node->next = temp->next;
  temp->next->prev = node;
  temp->next = node;

  syscall.list_size ++;
  syscall.total_time += node->time;
  syscall_check();
}

/* function syscall_delete_do()
 * delete the node
 */
static void syscall_delete_do(list_node *node){
  node->prev->next = node->next;
  node->next->prev = node->prev;

  node->prev = NULL;
  node->next = NULL;

  syscall.list_size --;
  syscall.total_time -= node->time;
  syscall_check();
}

/* function syscall_insert():interface
 * try to insert a new item into <list> syscall
 * If there is a node of the same name, plus the time, 
 * Otherwise, create a new node, set the name and time
 */
static void syscall_insert(char name[], double time) {
  // judge whether to insert or not
  list_node *node = syscall.head->next;
  while(node != syscall.tail && strcmp(name, node->name) != 0)
    node = node->next;

  if(node != syscall.tail) {
    assert(strcmp(name, node->name) == 0);
    // delete_do
    syscall_delete_do(node);
    
    node->time += time;

    // insert_do
    syscall_insert_do(node);
  }
  else {
    int index = syscall.list_size;
    list_node *current = &pool[index];
    current->id = index;
    strcpy(current->name, name);
    current->time = time;

    // insert_do
    syscall_insert_do(current);
  }
  // check
  syscall_check();
}

static int _files[2];

int main(int argc, char *argv[]) {
  /* exception handling */
  if(argc <= 1) {
    fprintf(stderr, "Error: \n");
    fprintf(stderr, "Please enter the command you want to trace. \n");
    return 1;
  }

  /* exception handling */
  if(pipe(_files) != 0) {
    fprintf(stderr, "Error: \n");
    fprintf(stderr, "Can't create pipes. \n");
    return 1;
  }

  syscall_init();

  int pid = fork();
  if(pid == 0) {
    char *filename = "/usr/bin/strace";
    char **newargv = (char **)malloc((argc + 1)*sizeof(char *));
    char **newenvp = {NULL};
    newargv[0] = "strace", newargv[1] = "-T";
    for(int i = 1; i < argc; i ++) newargv[i + 1] = argv[i];

    // aboard the stdout
    int trash = open("/dev/null", O_WRONLY, S_IWUSR);
    dup2(trash, STDOUT_FILENO);
    close(trash);

    // catch the stderr
    dup2(_files[1], STDERR_FILENO);
    close(_files[1]);
    execve(filename, newargv, newenvp);
    assert(0);
  }
  else {
    // change the stdin
    dup2(_files[0], STDIN_FILENO);
    close(_files[0]);

    char buf[4096];
    while(fgets(buf, 1024, stdin)){
      int len = strlen(buf);
      if(len > 2 && buf[len - 2] == '>') {
        int name_begin, name_end, time_begin, time_end;
        name_begin = name_end = 0;
        time_begin = time_end = len - 3;
        while(buf[name_end + 1] != '(') name_end ++;
        while(buf[time_begin - 1] != '<') time_begin --;
        char name[1024], time[1024];
        strncpy(name, buf + name_begin, name_end - name_begin + 1);
        strncpy(time, buf + time_begin, time_end - time_begin + 1);
        name[name_end - name_begin + 1] = '\0';
        time[time_end - time_begin + 1] = '\0';
        double _time; sscanf(time, "%lf", &_time);
        syscall_insert(name, _time);
        syscall_print();
      }
      buf[0]='\0';
      usleep(200000);
    }
  }
  return 0;
}
