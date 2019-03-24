#include <stdio.h>
#include <assert.h>
// extend
#include <stdlib.h>
#include <string.h> 
#include <dirent.h>

enum {VERSION = 1, NUMERIC_SORT = 2, SHOW_PIDS = 4};
void warning() {printf("Unsupported operation! \n");}

struct list_node{
  int child_pid;
  char child_name[64];
  struct list_node* next;
};

typedef struct list_node list_node;

struct list{
  list_node* head;
  list_node* tail;
};

typedef struct list list;

list Tree[(1 << 15)] = {}; // There are at most (1 << 15) process in Linux

int guard_cmp1(list_node* p, list_node* q, list_node* head, list_node* tail){
  if(p == head || q == tail)
    return 0;
  else
    return p->child_pid > q->child_pid;
}

int guard_cmp2(list_node* p, list_node* q, list_node* head, list_node* tail){
  if(p == head || q == tail)
    return 0;
  else
    return strcmp(p->child_name, q->child_name) > 0;
}

static void init_node(list_node* new_node, int pid, char name[]){
  new_node->child_pid = pid;
  strcpy(new_node->child_name, name);
}

static void insert_node_do(list_node* prev, list_node* mid, list_node* succ){
  prev->next = mid;
  mid->next = succ;
}

static void insert_node(int ppid, int pid, char name[], int (*cmp)(list_node*, list_node*, list_node*, list_node*)){
  if(Tree[ppid].head == NULL && Tree[ppid].tail == NULL){
    Tree[ppid].head = (list_node*)malloc(sizeof(list_node)); 
    Tree[ppid].tail = (list_node*)malloc(sizeof(list_node)); 
    Tree[ppid].head->next = Tree[ppid].tail;
  }

  list_node* new_node = (list_node*)malloc(sizeof(list_node));
  init_node(new_node, pid, name);
  
  list_node* p = Tree[ppid].head;
  list_node* p2 = p->next;
  while(p2 != Tree[ppid].tail && (*cmp)(new_node, p2, Tree[ppid].head, Tree[ppid].tail)){
      p = p->next;
      p2 = p2->next;
  }
  
  insert_node_do(p, new_node, p2);
}

static int dfs_mem[64] = {}, len_mem[64] = {};

void dfs_print_tree(int root, char name[], int depth, int max_valid, int show_pids){
  len_mem[depth] = show_pids ? printf("-%s(%d)", name, root) : printf("-%s", name);

  if(Tree[root].head != NULL)
  for(list_node* x = Tree[root].head->next; x != Tree[root].tail; x = x->next){
    if(x->next != Tree[root].tail){
      // update
      dfs_mem[depth] = 1, max_valid = depth;
    }
    else{
      // undo
      dfs_mem[depth] = 0, max_valid = 0;
      for(int i = 0; i < depth; i ++)
      if(dfs_mem[depth])
      max_valid = i;
    }

    dfs_print_tree(x->child_pid, x->child_name, depth + 1, max_valid, show_pids);

    if(x->next != Tree[root].tail){ 
      printf("\n");
      for(int i = 0; i <= max_valid; i ++){
        for(int j = 0; j < len_mem[i]; j ++)
        printf("%c", (dfs_mem[i] && j == len_mem[i]-1) ?
              (x->next->next == Tree[root].tail && i == max_valid ? '`' : '|') : ' ');
      }
    }
  }
}

void print_tree(int show_pids){
  printf("Process Tree: \n");
  list_node* x;
  for(x = Tree[0].head->next; x != Tree[0].tail; x = x->next)
    if(x->child_pid == 1) break;
  assert(x->child_pid == 1);
  dfs_print_tree(1, x->child_name, 0, 0, show_pids);
  printf("\n");
}

void read_proc_status_do(char filepath[], int (*cmp)(list_node*, list_node*, list_node*, list_node*)){
  FILE* fp = fopen(filepath, "r");
  if(fp){
    char temp[128], name[128]; 
    int tgid, pid, ppid;
    fgets(temp, 128, fp);
    fgets(name, 128, fp);
    fgets(name, 128, fp);
    fscanf(fp, "Tgid: %d\n", &tgid);
    fgets(name, 128, fp);
    fscanf(fp, "Pid: %d\n", &pid);
    fscanf(fp, "PPid: %d\n", &ppid);
    fclose(fp);

    strcpy(name, temp+6);
    name[strlen(name)-1]='\0';
    insert_node(tgid == pid ? ppid : tgid, pid, name, cmp);
  }
  else{

  }
}

void read_proc_status(char _pid[], int (*cmp)(list_node*, list_node*, list_node*, list_node*)){
  char filepath[128], filepath2[512];
  sprintf(filepath, "/proc/%s/task", _pid);
  DIR* dir = opendir(filepath);

  if(dir){
    struct dirent* ptr;
    while((ptr = readdir(dir))){
      if(strcmp(ptr->d_name, ".") == 0
            ||strcmp(ptr->d_name, "..") == 0
            ||ptr->d_type != DT_DIR)
      continue;

      sprintf(filepath2, "%s/%s/status", filepath, (char *)ptr->d_name);
      //printf("read: %s\n", filepath2);
      read_proc_status_do(filepath2, cmp);
    }
  }
  else{

  }
}

int main(int argc, char *argv[]) {

  int function_flag = 0; /* 4 for -p --show-pids
                          * 2 for -n --numeric-sort
                          * 1 for -V --version
                          */

  for (int i = 0; i < argc; i++) {
    assert(argv[i]); // always true

    if(i == 0)
    continue;

    if(argv[i][0] == '-' && argv[i][1] == '-'){
      if(strcmp(argv[i], "--show-pids") == 0)
      function_flag |= SHOW_PIDS;
      else if(strcmp(argv[i], "--numeric-sort") == 0)
      function_flag |= NUMERIC_SORT;
      else if(strcmp(argv[i], "--version") == 0)
      function_flag |= VERSION;
      else{
        warning(); return -1;
      }
    }
    else if(argv[i][0] == '-'){
      int len = strlen(argv[i]);
      for(int j = 1; j < len; j ++){
        switch(argv[i][j]){
          case 'p': function_flag |= SHOW_PIDS; break;
          case 'n': function_flag |= NUMERIC_SORT; break;
          case 'V': function_flag |= VERSION; break;
          default: warning(); return -1;
        }
      }
    }
    else{
      warning(); return -1;
    }
  }
  assert(!argv[argc]); // always true

  int execute(int function_flag);
  int ret = execute(function_flag);
  return ret;
}

int version(){
  const char author_name[] = "Lanceloia";
  const char author_email[] = "3509175458@qq.com";
  const char test_environment[] = "Ubuntu 4.18.0";
  const char update_date[] = "2019/2/28";

  printf("pstree version 1.0.0\n");
  printf("author: %s\n", author_name);
  printf("email: %s\n", author_email);
  printf("testing at: %s\n", test_environment);
  printf("last update: %s\n", update_date);
  return 0;
}

int execute(int function_flag){
  if(function_flag & VERSION)
  return version();

  DIR* dir = opendir("/proc");

  if(dir){
    struct dirent* ptr;
    while((ptr = readdir(dir))){
      if(strcmp(ptr->d_name, ".") == 0
            ||strcmp(ptr->d_name, "..") == 0
            ||ptr->d_type != DT_DIR)
      continue;

      if(function_flag & NUMERIC_SORT)
      read_proc_status(ptr->d_name, guard_cmp1);
      else
      read_proc_status(ptr->d_name, guard_cmp2);
    }
  }
  else{
    printf("Error\n"); return -1;
  }

  print_tree(function_flag & SHOW_PIDS);
  return 0;
}
