#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
//#include <sys/types.h>

#define KB *1024
#define MB *1024*1024

#define debug(name, val) fprintf(stderr, "%-32s %d\n", name, val)
#define debug2(str) fprintf(stderr, "%s\n", str)
#define debug3(name, str) fprintf(stderr, "%-32s %s\n", name, str)
#define debugx(name, val) fprintf(stderr, "%-32s 0x%x\n", name, val)

void *imgmap;

struct FAT32 {
  int sector_size;  // the amount of bytes per sector,       0x0b, 2
  int cluster_size; // the amount of sectors per cluster,    0x0d, 1
  int DOS_sec_amount; // the amount of DOS's sectors,        0x0e, 2
  int fat_amount;   // the amount of file allocation tables, 0x10, 1
  int sector_amount;// the amount of sectors,                0x13, 2
  int hiden_amount;  // the amount of hiden sectors,          0x1c, 4
  int fat_size;     // the amount of sector per FAT,         0x24, 4
  int DBR_sec_index;// the index of DBR sector,              0x32, 2
  char volume_label[12]; // the label of volime,             0x47, 11
  char fat_type[9]; // the type of FAT system,               0x52, 8
} fat32;

void print_FAT32_info() {
  return;

  debug("sector size", fat32.sector_size);
  debug("cluster size", fat32.cluster_size);
  debug("DOS sector amount", fat32.DOS_sec_amount);
  debug("fat amount", fat32.fat_amount);
  debugx("fat size(sectors)", fat32.fat_size);
  debugx("fat total_size(bytes)", fat32.fat_size * fat32.sector_size);
  debug("sector amount", fat32.sector_amount);
  debug("hiden_amount", fat32.hiden_amount);

  debug("DBR sec index", fat32.DBR_sec_index);
  debug3("volume lable", fat32.volume_label);
  debug3("fat type", fat32.fat_type);
}

int read_num(void *p, int len) {
  int ret = 0;
  for(int i = len - 1; i >= 0; i --) {
    ret = (ret << 8) + ((unsigned char *)p)[i];
  }
  return ret;
}

void read_fat32_info(char *data) {
  fat32.sector_size = read_num(data + 0x0b, 2);
  fat32.cluster_size = read_num(data + 0x0d, 1);
  fat32.DOS_sec_amount = read_num(data + 0x0e, 2);
  fat32.fat_amount = read_num(data + 0x10, 1);
  fat32.sector_amount = read_num(data + 0x13, 2);
  if(fat32.sector_amount == 0) fat32.sector_amount = read_num(data + 0x20, 4);
  fat32.hiden_amount = read_num(data + 0x1c, 4);
  fat32.fat_size = read_num(data + 0x24, 4);
  fat32.DBR_sec_index = read_num(data + 0x32, 2);
  strncpy(fat32.volume_label, data + 0x47, 11);
  strncpy(fat32.fat_type, data + 0x52, 8);
  print_FAT32_info();
} 

struct BMP_INFO {
  int offset, size;
  int color, width, height;
  char filename[256];
} BMP_INFO[128];

int tot_bmp = 0;

void read_unicode(char dest[], char src[], int len) {
  for(int i = 0; i < len; i ++) {
    dest[i] = src[2 * i];
  }
}

struct myFILE {
  char filename[256];
  int position, filesize;
  int next_sector, visited;
} file[1024];
int tot_file;

char buf[64][256] = {};

int top;

int is_valid(int key){
  if(0x01 <= key && key <= 0x1f)
    return 1;
  if(0x41 <= key && key <= 0x5f)
    return 1;
  return 0;
}

void read_name_position_do(char *data, int offset, struct myFILE *file) {
  if(*(data + offset + 0x0b) == (char)0x0f &&
    *(data + offset + 0x0C ) == (char)0x00 &&
   is_valid(read_num(data + offset, 1))) {
    read_unicode(buf[top], data + offset + 0x01, 5);
    read_unicode(buf[top] + 5, data + offset + 0x0e, 6);
    read_unicode(buf[top] + 11, data + offset + 0x1c, 2);
    top ++;
  }
  else {
    if(top) {
      while(top) {
          strcat(file->filename, buf[--top]);
      }

      file->position = read_num(data + offset + 0x14, 2) << 16;
      file->position += read_num(data + offset + 0x1a, 2);
      file->filesize = read_num(data + offset + 0x1c, 4);
      file->next_sector = file->position + file->filesize / fat32.sector_size + 1;
    }
    else {
      tot_file --;
    } 
  }
}

void read_name_position(char *data, int offset) {
  if(top == 0) {
    tot_file ++;
    read_name_position_do(data, offset, &file[tot_file - 1]);
  }
  else {
    read_name_position_do(data, offset, &file[tot_file - 1]);
  }
}

int sector_visit[(1 << 16) - 1];

int search_bmp_name_position(char *data, int offset, int sector_idx) {
  if (tot_file >= 512)
    return 0;
  
  if(sector_visit[sector_idx & 0xffff])
    return 0;
  sector_visit[sector_idx & 0xffff] = 1;
  
  int old_tot_file = tot_file;
  for(int i = 0; i < fat32.sector_size; i += 0x20) {
    read_name_position(data, offset + i);
  }
  return tot_file - old_tot_file;
}

void show_file(){
  for(int i = 0; i < tot_file; i ++){
    fprintf(stderr, "%d: filename: %s, position: 0x%08x, filesize: 0x%08x, next_sector: 0x%08x\n",
      i, file[i].filename, file[i].position, file[i].filesize, file[i].next_sector);
  }
}

void output_bmp(char *data,  struct myFILE *f){
  FILE *fp = fopen(f->filename, "wb");
  if(!fp) return;
  fwrite(data + (f->position - 0x2) * fat32.sector_size,
    sizeof(char), f->filesize, fp);
  fclose(fp);
}

void sha1sum_bmp(char *data,  struct myFILE *f){
  int _files[2];

  if(pipe(_files) != 0) {
    fprintf(stderr, "Error: \n");
    fprintf(stderr, "Can't create pipes. \n");
    return;
  }

  int pid = fork();
  if(pid == 0) {
    char *filename = "/usr/bin/sha1sum";
    char *newargv[] = {"sha1sum", NULL, NULL};
    char *newenvp[] = {NULL};
    
    newargv[1] = f->filename;

    // catch the stderr
    dup2(_files[1], STDERR_FILENO);
    close(_files[0]);
    execve(filename, newargv, newenvp);
  }
  else {
    // read data
    char buf[1024] = {};
    close(_files[1]);
    read(_files[0], buf, 1024);
    printf("%s", buf);
  }
}

void delete_bmp(struct myFILE *f){
  unlink(f->filename);
}

int deep_search_bmp_name_position(char *data, int offset) {
  int cnt = 0;
  int actual_tot_file = top == 0 ? tot_file - 1 : tot_file - 2; 
  for(int i = 0; i <= actual_tot_file; i ++)
    if(file[i].visited == 0) {
      file[i].visited = 1;
      if(file[i].next_sector <= 0xffff)
        cnt += search_bmp_name_position(
          data, offset + (file[i].next_sector - 2) * fat32.sector_size,
          file[i].next_sector);
    }
  return cnt;
}

int main(int argc, char *argv[]) {
  int fd = open(argv[1], O_RDWR);
  if (fd == -1) {debug2("open failed."); return 1;}
  imgmap = mmap(NULL, 64 MB, PROT_READ, MAP_SHARED, fd, 0);
  if (imgmap == (void *)-1) {debug2("mmap failed."); return 1;} 
  close(fd);

  read_fat32_info(imgmap);
  if(fat32.cluster_size != 1) {
    fprintf(stderr, "cluster_size is not equal to 1, this program maybe error!\n");
  }

  int fat_begin = 0;
  while(strncmp(imgmap + fat_begin, "\xf8\xff\xff\x0f", 4) != 0)
    fat_begin += fat32.sector_size;
  int fat_tot_size = fat32.fat_amount * fat32.fat_size * fat32.sector_size;
  int data_start_sector = (fat_begin + fat_tot_size) / fat32.sector_size;

  search_bmp_name_position(imgmap, (data_start_sector + (3 - 2)) * fat32.sector_size, 3);
  
  while(deep_search_bmp_name_position(imgmap, fat_begin + fat_tot_size));

  for(int i = 4; (data_start_sector + (i - 2)) < fat32.sector_amount; i ++) {
    search_bmp_name_position(imgmap, (data_start_sector + (i - 2)) * fat32.sector_size, i);
  }  

  show_file();

  int actual_tot_file = top == 0 ? tot_file - 1 : tot_file - 2;   
  for(int i = 0; i <= actual_tot_file; i ++) {
    output_bmp(imgmap + fat_begin + fat_tot_size, &file[i]);
    sha1sum_bmp(imgmap + fat_begin + fat_tot_size, &file[i]);
    delete_bmp(&file[i]);
  }
   
  return 0;
}
