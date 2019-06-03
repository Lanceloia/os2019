#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

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
  int fat_size;     // the amount of sector per FAT,         0x24, 4
  int DBR_sec_index;// the index of DBR sector,              0x32, 2
  char volume_label[12]; // the label of volime,             0x47, 11
  char fat_type[9]; // the type of FAT system,               0x52, 8
} fat32;

void print_FAT32_info() {
  debug("sector size", fat32.sector_size);
  debug("cluster size", fat32.cluster_size);
  debug("DOS sector amount", fat32.DOS_sec_amount);
  debug("fat amount", fat32.fat_amount);
  debugx("fat size(sectors)", fat32.fat_size);
  debugx("fat total_size(bytes)", fat32.fat_size * fat32.sector_size);
  debug("sector amount", fat32.sector_amount);

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
  fat32.fat_size = read_num(data + 0x24, 4);
  fat32.DBR_sec_index = read_num(data + 0x32, 2);
  strncpy(fat32.volume_label, data + 0x47, 11);
  strncpy(fat32.fat_type, data + 0x52, 8);
  print_FAT32_info();
} 

struct YELLO_BMP {
  int color, clusters_size;
  char filename[256];
  int clusters[1024];
} yello_bmp[128];

int tot_bmp = 0;

void push_cluster(struct YELLO_BMP *yb, int offset) {
  yb->clusters[yb->clusters_size ++] = offset;
}

int read_unicode(char dest[], char src[], int len) {
  for(int i = 0; i < len; i ++) {
    dest[i] = src[2 * i];
    if(dest[i] == '\0')
      return 0;
  }
  return 1;
}

char filename[256][256]= {};
char buf[256] = {};
int tot_fn;

void read_name(char *data, int offset) {
  if(*(data + offset + 0x00) == (char)0x01 &&
  *(data + offset + 0x0b) == (char)0x0f) {
    if(read_unicode(buf, data + offset + 0x01, 5))
      if(read_unicode(buf + 5, data + offset + 0x0e, 6))
        read_unicode(buf + 11, data + offset + 0x1c, 2);

    strcpy(filename[tot_fn ++ ], buf);
  }
  /*
  else {
    // short filename
    strncpy(dest, (char *)(data + offset), 8);
  }
  */
}

int search_bmp_name(char *data, int offset) {
  for(int i = 0; i < fat32.sector_size; i += 0x20){
    read_name(data, offset + i);
  }
  return 0;
}

void search_bmp_head(char *data, int offset) {
  if(data[offset] == 'B' && data[offset + 1] == 'M') {
    // init_yello_bmp
    yello_bmp[tot_bmp].color = read_num(data + offset + 0x54, 3);
    push_cluster(&yello_bmp[tot_bmp], offset);
    tot_bmp ++;
  }
}

void show_filename(){
  for(int i = 0; i < 256; i ++){
    printf("%d: %s\n", i, filename[i]);
  }
}

void show_yello_bmp(){
  for(int i = 0; i < 128; i ++) {
    if(yello_bmp[i].clusters_size == 0)
      break;
    printf("bmp_index: %d,", i);
    printf("color: %x,", yello_bmp[i].color);
    printf("clusters_size: %d\n", yello_bmp[i].clusters_size);
  }
}


int judge_attribution(void *data, int offset) {
  return 0;
}

int main(int argc, char *argv[]) {
  int fd = open(argv[1], O_RDWR);
  if (fd == -1) {debug2("open failed."); return 1;}
  imgmap = mmap(NULL, 64 MB, PROT_READ, MAP_SHARED, fd, 0);
  if (imgmap == (void *)-1) {debug2("mmap failed."); return 1;} 
  close(fd);
  read_fat32_info(imgmap);
  
  for(int i = 0; i < 32 MB; i += fat32.sector_size) {
    switch (judge_attribution(imgmap, i)) {
    case 0: // ctrl-sector
      search_bmp_name(imgmap, i);
      break;
    case 1: // data-sector
      search_bmp_head(imgmap, i);
      break;
    default:
      break;
    }

/*
    if(search_bmp_head(imgmap, i))
      ;
    else {
      int len = 0;
      for(int j = 0; j < fat32.sector_size; j += 0x20) {
        int ret = read_name(imgmap, i + j, buf + len);
        if(0 < ret && ret < 13){
          printf("%s\n", buf);
          len = 0;
        }
        else if (ret == 13)
          len += ret;
        //else
        //  len = 0;
      }
    }
*/
  }
  show_fn();
  show_yello_bmp();

  return 0;
}
