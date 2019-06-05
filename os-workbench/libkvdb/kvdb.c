#include "kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THREAD_LOCK
#define FILE_LOCK

#ifdef FILE_LOCK
    int readlock(int fd) {
        struct flock lock;
        lock.l_type = F_RDLCK; lock.l_whence = SEEK_SET;
        lock.l_start = 0;      lock.l_len = 0;
        lock.l_pid = getpid();
        if(fcntl(fd, F_SETLKW, &lock) == 0)
            return 1;
        return 0;
    }
    int writelock(int fd) {
        struct flock lock;
        lock.l_type = F_WRLCK; lock.l_whence = SEEK_SET;
        lock.l_start = 0;      lock.l_len = 0;
        lock.l_pid = getpid();
        if(fcntl(fd, F_SETLKW, &lock) == 0)
            return 1;
        return 0;
    }
    int unlock(int fd) {
        struct flock lock;
        lock.l_type = F_UNLCK; lock.l_whence = SEEK_SET;
        lock.l_start = 0;      lock.l_len = 0;
        lock.l_pid = getpid();
        if(fcntl(fd, F_SETLKW, &lock) == 0)
            return 1;
        return 0;
    }

    #define myfopen(fp, filename, mode) \
    do { \
        fp = fopen(filename, mode);\
        readlock(fileno(fp));\
        writelock(fileno(fp));\
    }while(0)
    #define myfclose(fp) \
    do { \
        unlock(fileno(fp));\
        fclose(fp);\
    }while(0)
#else
    #define myfopen(fp, filename, mode) \
    do { \
        fp = fopen(filename, mode); \
    }while(0)
    #define myfclose(fp) \
        do { \
        fclose(fp);\
    }while(0)
#endif

struct flock flk;

static int kvdb_get_unused_idx(kvdb_t *db, int k) {
    if(k < 0) return -1;
    for(int i = k; i < DATAS_SIZE; i ++) 
        if(db->datas[i].used == 0) return i;
    panic("all datas are used. \n");
    return -1;
}

static int vec[MAX_LENGTH / DATA_LENGTH];
static int kvdb_alloc(kvdb_t *db, int size, const char *key) {
    int cnt = 0, k = -1, need = (size + DATA_LENGTH-1) / DATA_LENGTH;
    for(int i = 0; i < need; i ++) {
        k = kvdb_get_unused_idx(db, k + 1);
        if(k == -1) { panic("no enough space. \n"); return 1; }
        vec[cnt ++] = k;
    }
    if(cnt != need) { panic("something error. \n"); return 1; }
    vec[cnt] = -1;
    for(int i = 0; i < need; i ++) {
        db->datas[vec[i]].used = 1;
        db->datas[vec[i]].next = vec[i + 1];
    }
    db->items[db->size].head = vec[0];
    db->items[db->size].size = size;
    strcpy(db->items[db->size].key, key);
    db->size ++;
    return 0;
}

static int kvdb_free(kvdb_t *db, int item_idx) {
    item_t *itm = &db->items[item_idx];
    int need = (itm->size + DATA_LENGTH-1) / DATA_LENGTH;
    int x = itm->head;
    for(int i = 0; i < need; i ++) {
        db->datas[x].used = 0;
        x = db->datas[x].next;
    }
    if(x != -1) {panic("something error. \n"); return 1;}
    itm->head = db->items[db->size - 1].head;
    itm->size = db->items[db->size - 1].size;
    strcpy(itm->key, db->items[db->size - 1].key);
    db->size --;
    return 0;
}

char readbuf[MAX_LENGTH / DATA_LENGTH][DATA_LENGTH];
static int kvdb_read(kvdb_t *db, int item_idx) {
    item_t *itm = &db->items[item_idx];
    int cnt = 0, need = (itm->size + DATA_LENGTH-1) / DATA_LENGTH;
    int x = itm->head;
    for(int i = 0; i < need; i ++) {
        strncpy(readbuf[cnt++], db->datas[x].data, DATA_LENGTH);
        x = db->datas[x].next;
    }
    if(x != -1) {panic("something error. \n"); return 1;}
    return 0;
}

char writebuf[MAX_LENGTH / DATA_LENGTH][DATA_LENGTH];
static int kvdb_write(kvdb_t *db, int item_idx) {
    item_t *itm = &db->items[item_idx];
    int cnt = 0, need = (itm->size + DATA_LENGTH-1) / DATA_LENGTH;
    int x = itm->head;
    for(int i = 0; i < need; i ++) {
        strncpy(db->datas[x].data, writebuf[cnt++], DATA_LENGTH);
        x = db->datas[x].next;
    }
    if(x != -1) {panic("something error. \n"); return 1;}
    // record here
    return 0;   
}

static int kvdb_search_key(kvdb_t *db, const char *key) {
    for(int i = 0; i < db->size; i ++) {
        if(strcmp(db->items[i].key, key) == 0)
            return i;
    }
    return -1;
}

static void kvdb_put_redo(kvdb_t *db, const char *key, const char *value) {
    int item_idx = kvdb_search_key(db, key);
    int size = strlen(value);
    if(item_idx >= 0) { kvdb_free(db, item_idx); }
    if(kvdb_alloc(db, size, key)) {exit(1);}
    strcpy((char *)writebuf, value);
    if(kvdb_write(db, db->size - 1)) {exit(1);}
}

static void kvdb_get_redo(kvdb_t *db, const char *key) {
    int item_idx = kvdb_search_key(db, key);
    if(item_idx == -1) {return;}
    if(kvdb_free(db,item_idx)) {exit(1);}
}

char operation[4], key[KEY_LENGTH], value[MAX_LENGTH + 1];
static int redo_operation(FILE *lp, kvdb_t *db) {
    while(fscanf(lp, "%3s %s ", operation, key) == 2) {
        if(strcmp(operation, "put") == 0) {
            fgets(value, MAX_LENGTH, lp); value[strlen(value) - 1] = '\0';
            kvdb_put_redo(db, key, value);
        }
        else if(strcmp(operation, "get") == 0) {
            kvdb_get_redo(db, key);
        }
        else {
            panic("invaild log. \n");
            exit(1);
        }
    }
    // no operation access
    return 0;
}

int kvdb_open(kvdb_t *db, const char *filename) {
    int len = strlen(filename);
    strncpy(db->filename, filename, len);
    strncpy(db->copyname, filename, len - 3);
    strncpy(db->logname, filename, len - 3);
    strcat(db->copyname, ".dbc");
    strcat(db->logname, ".log");
    FILE *fp, *cp, *lp;

    if(!access(db->copyname, R_OK) && !access(db->logname, R_OK)) {
        // recover
        myfopen(fp, db->filename, "wb");
        myfopen(cp, db->copyname, "rb");
        myfopen(lp, db->logname, "rt");
        if(fp == NULL || cp == NULL || lp == NULL) { return 1; }
        fread(db, sizeof(kvdb_t), 1, cp);
        while(redo_operation(lp, db));
        fwrite(db, sizeof(kvdb_t), 1, fp);
        myfclose(lp);
        myfclose(cp);
        myfclose(fp);

        myfopen(lp, db->logname, "wt");
        if(lp == NULL) { return 1; }
        myfclose(lp);
    }
    else if(!access(db->filename, R_OK)){
        // open
        myfopen(fp, db->filename, "rb");
        myfopen(cp, db->copyname, "wb");
        myfopen(lp, db->logname, "wt");
        if(fp == NULL || cp == NULL || lp == NULL) { return 1; }
        fread(db, sizeof(kvdb_t), 1, fp);
        fwrite(db, sizeof(kvdb_t), 1, cp);
        myfclose(lp);
        myfclose(cp);
        myfclose(fp);
    }
    else {
        // create
        myfopen(fp, db->filename, "wb");
        myfopen(cp, db->copyname, "wb");
        myfopen(lp, db->logname, "wt");
        if(fp == NULL || cp == NULL || lp == NULL) { return 1; }
        fwrite(db, sizeof(kvdb_t), 1, fp);
        fwrite(db, sizeof(kvdb_t), 1, cp);
        myfclose(lp);
        myfclose(cp);
        myfclose(fp);
    }

#ifdef THREAD_LOCK
    if(pthread_mutex_init(&db->tlock, NULL)) {
        panic("mutex_lock_init failed. \n");
        return 1;
    }
#endif

    return 0;
}

int kvdb_close(kvdb_t *db) {
    // save
    FILE *fp;
    myfopen(fp, db->filename, "wb");
    if(fp == NULL) { return 1; }
    fwrite(db, sizeof(kvdb_t), 1, fp);
    myfclose(fp);

    unlink(db->copyname);
    unlink(db->logname);

#ifdef THREAD_LOCK
    pthread_mutex_destroy(&db->tlock);
#endif
    return 0;
}

int kvdb_put(kvdb_t *db, const char *key, const char *value) {
#ifdef THREAD_LOCK
    pthread_mutex_lock(&db->tlock);
#endif

    int item_idx = kvdb_search_key(db, key);
    int size = strlen(value);
    if(item_idx >= 0) { kvdb_free(db, item_idx); }
    if(kvdb_alloc(db, size, key)) {
        panic("alloc failed. \n");
        goto PUT_FAILED;
    }
    strcpy((char *)writebuf, value);
    if(kvdb_write(db, db->size - 1)) {
        panic("write failed. \n");
        goto PUT_FAILED;
    }
    
    FILE *lp;
    myfopen(lp, db->logname, "at");
    if(lp == NULL) { exit(1); }
    fprintf(lp, "put %s %s\n", key, value);
    myfclose(lp);

PUT_SUCCESS:
#ifdef THREAD_LOCK
    pthread_mutex_unlock(&db->tlock);
#endif
    return 0;

PUT_FAILED:
#ifdef THREAD_LOCK
        pthread_mutex_unlock(&db->tlock); 
#endif
        return 1;
}

char *kvdb_get(kvdb_t *db, const char *key) {
#ifdef THREAD_LOCK
    pthread_mutex_lock(&db->tlock); 
#endif

    int item_idx = kvdb_search_key(db, key);
    if(item_idx == -1) {
        goto GET_FAILED;
    }
    if(kvdb_read(db,item_idx)) {
        panic("read failed. \n"); 
        goto GET_FAILED;
    }
    if(kvdb_free(db,item_idx)) {
        panic("free failed. \n");
        goto GET_FAILED;
    }

    FILE *lp;
    myfopen(lp, db->logname, "at");
    if(lp == NULL) { exit(1); }
    fprintf(lp, "get %s \n", key);
    myfclose(lp);

GET_SUCCESS:
#ifdef THREAD_LOCK
    pthread_mutex_unlock(&db->tlock); 
#endif
    return (char *)readbuf;

GET_FAILED:
#ifdef THREAD_LOCK
    pthread_mutex_unlock(&db->tlock); 
#endif
    return NULL;
}