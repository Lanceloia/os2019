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
  int offset;
  int clusters_size;
  int color, width, height;
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

int is_valid(char ch) {
  if('a' <= ch && ch <= 'z') return 1;
  if('A' <= ch && ch <= 'Z') return 1;
  if('0' <= ch && ch <= '9') return 1;
  if(ch == '.' || ch == '_' || ch == '\0') return 1;
  return 0;
}

struct FILE {
  char filename[256];
  int offset;
} file[256];
int tot_file;

void read_name_offset(char *data, int offset) {
  if(*(data + offset + 0x0b) == (char)0x0f) {
    char buf[256] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    if(read_unicode(buf, data + offset + 0x01, 5))
      if(read_unicode(buf + 5, data + offset + 0x0e, 6))
        read_unicode(buf + 11, data + offset + 0x1c, 2);

    if(is_valid(buf[0]) && is_valid(buf[1]) && is_valid(buf[2])) {
      strcpy(file[tot_file].filename, buf);
      file[tot_file].offset = read_num(data + offset + 0x34, 2) << 16;
      file[tot_file ++].offset += read_num(data + offset + 0x3a, 2);
    
      if(strncmp(data + offset + 0x20, buf, 3) != 0) {
        printf("%s\n", buf);
      }
    }
  }
  else {
    if(*(data + offset + 0x06) == '~' && *(data + offset + 0x07) == '1') {
      //printf("%s\n", data + offset);
    }
  }
}

int search_bmp_name(char *data, int offset) {
  for(int i = 0; i < fat32.sector_size; i += 0x20){
    read_name_offset(data, offset + i);
  }
  return 0;
}

void search_bmp_head(char *data, int offset) {
  for(int j = 0; j < fat32.sector_size; j += 0x20) {
    if(data[offset + j] == 'B' && data[offset + j + 1] == 'M') {
      // init_yello_bmp
      yello_bmp[tot_bmp].color = read_num(data + offset + j + 0x54, 3);
      yello_bmp[tot_bmp].width = read_num(data + offset + 0x12, 4);
      yello_bmp[tot_bmp].height = read_num(data + offset + 0x16, 4);
      yello_bmp[tot_bmp].offset = offset + j;
      for(int i = 0; i < tot_file; i ++)
        if (file[i].offset == ((yello_bmp[tot_bmp].offset - 0x81c00) / 0x200))
          strcpy(yello_bmp[tot_bmp].filename, file[i].filename);
      push_cluster(&yello_bmp[tot_bmp], offset);
      tot_bmp ++;
    }
  }
}

void show_file(){
  for(int i = 0; i < tot_file; i ++){
    printf("%d: %s, 0x%08x\n",
    i, file[i].filename, file[i].offset);
  }
}

void show_yello_bmp(){
  for(int i = 0; i < tot_bmp; i ++) {
    if(yello_bmp[i].clusters_size == 0)
      break;
    printf("bmp_index: %d, ", i);
    printf("color: 0x%x, ", yello_bmp[i].color);
    printf("w: %d, h: %d, ", yello_bmp[i].width, yello_bmp[i].height);
    printf("offset: 0x%x, ", yello_bmp[i].offset);
    printf("filename: %s, ", yello_bmp[i].filename);
    printf("clusters_size: %d\n", yello_bmp[i].clusters_size);
  }
}

int judge_attribution(void *data, int offset) {
  if(((char *)data)[offset] == 'B' && ((char *)data)[offset + 1] == 'M') {
    return 0;
  }
  else if(read_num(data + offset, 3) == 
    read_num(data + offset + 0x3, 3) &&
    read_num(data + offset + 0x3, 3) ==
    read_num(data + offset + 0x6, 3) &&
    read_num(data + offset + 0x6, 3) ==
    read_num(data + offset + 0x9, 3) &&
    read_num(data + offset + 0x9, 3) ==
    read_num(data + offset + 0xc, 3) &&
    read_num(data + offset, 3) != 0x0
  )
    return 1;
  else
    return 2;
}

void output_bmp(char *data, struct YELLO_BMP *yb){
  FILE *fp = fopen(yb->filename, "wb+");
  if(!fp) return;
  printf("%x\n", *(int *)(data + yb->clusters[0]));
  char head[54];
  for(int i = 0; i < 54; i ++) {
    head[i] = *(data + yb->clusters[0] + i);
    // printf("%c", head[i]);
  }
  fwrite(head, sizeof(head), 1, fp);
  char color[3] = {};
    /*
    color[0] = (yb->color) & 0xff;
    color[1] = (yb->color >> 8) & 0xff;
    color[2] = (yb->color >> 16) & 0xff;
    */ 
  for(int i = 0; i < yb->height; i ++)
    for(int j = 0; j <  ((yb->width * 24 + 31) / 8); j ++)
      fwrite(color + (j % 3), sizeof(char), 1, fp);
  fclose(fp);
}

int main(int argc, char *argv[]) {
  int fd = open(argv[1], O_RDWR);
  if (fd == -1) {debug2("open failed."); return 1;}
  imgmap = mmap(NULL, 64 MB, PROT_READ, MAP_SHARED, fd, 0);
  if (imgmap == (void *)-1) {debug2("mmap failed."); return 1;} 
  close(fd);
  read_fat32_info(imgmap);
  
  for(int i = 0; i < 32 MB; i += fat32.sector_size) {
    if(judge_attribution(imgmap, i) == 2)
      search_bmp_name(imgmap, i);
  }

  for(int i = 0; i < 32 MB; i += fat32.sector_size) {
    if(judge_attribution(imgmap, i) == 0)
      search_bmp_head(imgmap, i);
  }

  show_file();
  show_yello_bmp();

  for(int i = 0; i < 16; i ++)
    output_bmp(imgmap, &yello_bmp[i]);
  return 0;
}
