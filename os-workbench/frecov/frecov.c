#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define KB *1024
#define MB *1024*1024

#define debug(name, val) fprintf(stderr, "%-16s %d\n", name, val)
#define debug2(str) fprintf(stderr, "%s\n", str)
#define debug3(name, str) fprintf(stderr, "%-16s %s\n", name, str)

void *imgmap;

struct FAT32 {
  int sector_size;  // the amount of bytes per sector,       0x0b, 2
  int cluster_size; // the amount of sectors per cluster,    0x0d, 1
  int DOS_sec_amount; // the amount of DOS's sectors,        0x0e, 2
  int fat_amount;   // the amount of file allocation tables, 0x10, 1
  int sector_amount;// the amount of sectors,                0x13, 2
  int DBR_sec_index;// the index of DBR sector,              0x32, 2
  char volume_label[12]; // the label of volime,             0x47, 11
  char fat_type[9]; // the type of FAT system,               0x52, 8
} fat32;

void print_FAT32_info(struct FAT32 *f) {
  debug("sector_size", f->sector_size);
  debug("cluster_size", f->cluster_size);
  debug("DOS_sec_amount", f->DOS_sec_amount);
  debug("fat_amount", f->fat_amount);
  debug("sector_amount", f->sector_amount);
  debug("DBR_sec_index", f->DBR_sec_index);
  debug3("volume_lable", f->volume_label);
  debug3("fat_type", f->fat_type);
}

int read_num(void *p, int len) {
  int ret = 0;
  for(int i = len - 1; i >= 0; i --) {
    ret = (ret << 8) + ((unsigned char *)p)[i];
  }
  return ret;
}

void read_fat32_info(struct FAT32 *f, void *data) {
  f->sector_size = read_num(data + 0x0b, 2);
  f->cluster_size = read_num(data + 0x0d, 1);
  f->DOS_sec_amount = read_num(data + 0x0e, 2);
  f->fat_amount = read_num(data + 0x10, 1);
  f->sector_amount = read_num(data + 0x13, 2);
  f->DBR_sec_index = read_num(data + 0x32, 2);
  strncpy(f->volume_label, data + 0x47, 11);
  strncpy(f->fat_type, data + 0x52, 8);
  print_FAT32_info(f);
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

void init_yello_bmp(void *data, int offset){
  yello_bmp[tot_bmp].color = read_num(data + offset + 0x54, 3);
  push_cluster(&yello_bmp[tot_bmp], offset);
  tot_bmp ++;
}

int search_yello_bmp(void *data, int offset){
  if(((char *)data)[offset] == 'B' && ((char *)data)[offset + 1] == 'M') {
    init_yello_bmp(data, offset);
  }
  return 0;
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


int main(int argc, char *argv[]) {
  int fd = open(argv[1], O_RDWR);
  if (fd == -1) {debug2("open failed."); return 1;}
  imgmap = mmap(NULL, 64 MB, PROT_READ, MAP_SHARED, fd, 0);
  if (imgmap == (void *)-1) {debug2("mmap failed."); return 1;} 
  close(fd);

  read_fat32_info(&fat32, imgmap);
  for(int i = 0; i < 12 MB; i += 0x200) {
    search_yello_bmp(imgmap, i);
  }
  show_yello_bmp();

  return 0;
}
