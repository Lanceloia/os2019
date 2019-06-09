#ifndef __KVDB_H__
#define __KVDB_H__
#include <pthread.h>
#include <sys/fcntl.h>

#define KEY_LENGTH (256)
#define FILENAME_LENGTH (256)
#define ITEMS_SIZE (256)
#define DATAS_SIZE (12 * 1024)

#define KB *1024
#define MB *1024*1024
#define DATA_LENGTH (4 KB)
#define MAX_LENGTH (32 MB)

#define panic(str) fprintf(stderr, "%s", str)

typedef struct node {
    char data[DATA_LENGTH];
    int used, next;
}node_t;

typedef struct item {
    int head, size;
    char key[KEY_LENGTH];
}item_t;

typedef struct kvdb {
    char filename[FILENAME_LENGTH];
    char copyname[FILENAME_LENGTH];
    char logname[FILENAME_LENGTH];
    int size;
    pthread_mutex_t tlock;
    item_t items[ITEMS_SIZE];
    node_t datas[DATAS_SIZE];
}kvdb_t;

int kvdb_open(kvdb_t *db, const char *filename);

int kvdb_close(kvdb_t *db);

int kvdb_put(kvdb_t *db, const char *key, const char *value);

char *kvdb_get(kvdb_t *db, const char *key);

#endif
