#include <fs.h>
#include <klib.h>
#include <simext2.h>

/* functions
 * copyright: leungjyufung2019@outlook.com
 */
void ext2_rd_sb(ext2_t* ext2);
void ext2_wr_sb(ext2_t* ext2);
void ext2_rd_gd(ext2_t* ext2);
void ext2_wr_gd(ext2_t* ext2);
void ext2_rd_ind(ext2_t* ext2, uint32_t i);
void ext2_wr_ind(ext2_t* ext2, uint32_t i);
void ext2_rd_dir(ext2_t* ext2, uint32_t i);
void ext2_wr_dir(ext2_t* ext2, uint32_t i);
void ext2_rd_blockbitmap(ext2_t* ext2);
void ext2_wr_blockbitmap(ext2_t* ext2);
void ext2_rd_inodebitmap(ext2_t* ext2);
void ext2_wr_inodebitmap(ext2_t* ext2);
void ext2_rd_datablock(ext2_t* ext2, uint32_t i);
void ext2_wr_datablock(ext2_t* ext2, uint32_t i);
uint32_t ext2_alloc_block(ext2_t* ext2);
uint32_t ext2_alloc_inode(ext2_t* ext2);
uint32_t ext2_reserch_file(ext2_t* ext2, char* name, int file_type,
                           uint32_t* inode_num, uint32_t* block_num,
                           uint32_t* dir_num);
void ext2_dir_prepare(ext2_t* ext2, uint32_t idx, uint32_t len, int type);
void ext2_remove_block(ext2_t* ext2, uint32_t del_num);
int ext2_search_file(ext2_t* ext2, uint32_t idx);

id_t* ext2_lookup(fs_t* fs, const char* path, int flags) { return NULL; };
int ext2_close(id_t* id) { return 0; }

void ext2_init(fs_t* fs, const char* name, device_t* dev) {
  ext2_t* ext2 = (ext2_t*)fs->fs;
  ext2->dev = dev;
  printf("Creating ext2fs\n");
  ext2->last_alloc_inode = 1;
  ext2->last_alloc_block = 0;
  for (int i = 0; i < MAX_OPEN_FILE_AMUT; i++) ext2->file_open_table[i] = 0;
  // for (int i = 0; i < BLK_SIZE; i++) ext2->datablockbuf = 0;
  memset(&ext2->sb, 0x00, SB_SIZE);
  memset(&ext2->gdt, 0x00, GD_SIZE * 1);
  ext2_wr_ind(ext2, 1);
  ext2_wr_dir(ext2, 0);
  strcpy(ext2->current_dir_name, "[root@ /");
  ext2_rd_gd(ext2);  // gdt is changed here
  ext2->gdt.block_bitmap = BLK_BITMAP;
  ext2->gdt.inode_bitmap = IND_BITMAP;
  ext2->gdt.inode_table = INDT_START;  // maye be no use
  ext2->gdt.free_blocks_count = DATA_BLOCK_COUNT;
  ext2->gdt.free_inodes_count = INODE_TABLE_COUNT;
  ext2->gdt.used_dirs_count = 0;
  ext2_wr_gd(ext2);

  ext2_rd_blockbitmap(ext2);
  ext2_rd_inodebitmap(ext2);
  ext2->ind.mode = 01006;
  ext2->ind.blocks = 0;
  ext2->ind.blocks = 32;  // maybe wrong
  ext2->ind.block[0] = ext2_alloc_block(ext2);
  ext2->ind.blocks++;
  ext2->current_dir = ext2_alloc_inode(ext2);
  ext2_wr_ind(ext2, ext2->current_dir);

  ext2->dir[0].inode = ext2->dir[1].inode = ext2->current_dir;
  ext2->dir[0].name_len = ext2->dir[1].name_len = 0;
  ext2->dir[0].file_type = ext2->dir[1].file_type = TYPE_DIR;
  strcpy(ext2->dir[0].name, ".");
  strcpy(ext2->dir[1].name, "..");
  ext2_wr_dir(ext2, ext2->ind.block[0]);
}

void ext2_rd_sb(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, DISK_START, &ext2->sb, SB_SIZE);
}

void ext2_wr_sb(ext2_t* ext2) {
  ext2->dev->ops->write(ext2->dev, DISK_START, &ext2->sb, SB_SIZE);
}

void ext2_rd_gd(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, GDT_START, &ext2->gdt, GD_SIZE);
}

void ext2_wr_gd(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, GDT_START, &ext2->gdt, GD_SIZE);
}

void ext2_rd_ind(ext2_t* ext2, uint32_t i) {
  uint32_t offset = INDT_START + (i - 1) * IND_SIZE;
  ext2->dev->ops->read(ext2->dev, offset, &ext2->ind, IND_SIZE);
}

void ext2_wr_ind(ext2_t* ext2, uint32_t i) {
  uint32_t offset = INDT_START + (i - 1) * IND_SIZE;
  ext2->dev->ops->write(ext2->dev, offset, &ext2->ind, IND_SIZE);
}

void ext2_rd_dir(ext2_t* ext2, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  ext2->dev->ops->read(ext2->dev, offset, &ext2->dir, BLK_SIZE);
}

void ext2_wr_dir(ext2_t* ext2, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  ext2->dev->ops->write(ext2->dev, offset, &ext2->dir, BLK_SIZE);
}

void ext2_rd_blockbitmap(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, BLK_BITMAP, &ext2->blockbitmapbuf, BLK_SIZE);
}

void ext2_wr_blockbitmap(ext2_t* ext2) {
  ext2->dev->ops->write(ext2->dev, BLK_BITMAP, &ext2->blockbitmapbuf, BLK_SIZE);
}

void ext2_rd_inodebitmap(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, IND_BITMAP, &ext2->inodebitmapbuf, BLK_SIZE);
}

void ext2_wr_inodebitmap(ext2_t* ext2) {
  ext2->dev->ops->write(ext2->dev, IND_BITMAP, &ext2->inodebitmapbuf, BLK_SIZE);
}

void ext2_rd_datablock(ext2_t* ext2, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  ext2->dev->ops->read(ext2->dev, offset, &ext2->datablockbuf, BLK_SIZE);
}

void ext2_wr_datablock(ext2_t* ext2, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  ext2->dev->ops->write(ext2->dev, offset, &ext2->datablockbuf, BLK_SIZE);
}

uint32_t ext2_alloc_block(ext2_t* ext2) {
  uint32_t cur = ext2->last_alloc_block / 8;
  uint32_t con = 0x80; /* 0b10000000 */
  int flag = 0;
  if (ext2->gdt.free_blocks_count == 0) {
    return -1;
  }
  ext2_rd_blockbitmap(ext2);
  while (ext2->blockbitmapbuf[cur] == 0xff) {
    if (cur == BLK_SIZE - 1)
      cur = 0;  // restart from zero
    else
      cur++;  // try next
  }
  while (ext2->blockbitmapbuf[cur] & con) {
    con = con / 2;
    flag++;
  }
  ext2->blockbitmapbuf[cur] = ext2->blockbitmapbuf[cur] + con;
  ext2->last_alloc_block = cur * 8 + flag;
  ext2_wr_blockbitmap(ext2);
  ext2->gdt.free_blocks_count--;
  ext2_wr_gd(ext2);
  return ext2->last_alloc_block;
}

uint32_t ext2_alloc_inode(ext2_t* ext2) {
  uint32_t cur = (ext2->last_alloc_inode - 1) / 8;
  uint32_t con = 0x80; /* 0b10000000 */
  int flag = 0;
  if (ext2->gdt.free_inodes_count == 0) {
    return -1;
  }
  ext2_rd_inodebitmap(ext2);
  while (ext2->inodebitmapbuf[cur] == 0xff) {
    if (cur == BLK_SIZE - 1)
      cur = 0;  // restart from zero
    else
      cur++;  // try next
  }
  while (ext2->inodebitmapbuf[cur] & con) {
    con = con / 2;
    flag++;
  }
  ext2->inodebitmapbuf[cur] = ext2->inodebitmapbuf[cur] + con;
  ext2->last_alloc_inode = cur * 8 + flag + 1;  // why add 1 here?
  ext2_wr_inodebitmap(ext2);
  ext2->gdt.free_inodes_count--;
  ext2_wr_gd(ext2);
  return ext2->last_alloc_inode;
}

uint32_t ext2_reserch_file(ext2_t* ext2, char* name, int file_type,
                           uint32_t* inode_num, uint32_t* block_num,
                           uint32_t* dir_num) {
  ext2_rd_ind(ext2, ext2->current_dir);
  for (uint32_t j = 0; j < ext2->ind.blocks; j++) {
    ext2_rd_dir(ext2, ext2->ind.block[j]);
    for (uint32_t k = 0; k < 32;) {
      if (!ext2->dir[k].inode || ext2->dir[k].file_type != file_type ||
          strcmp(ext2->dir[k].name, name)) {
        k++;
      } else {
        *inode_num = ext2->dir[k].inode;
        *block_num = j;
        *dir_num = k;
        return 1;
      }
    }
  }
  return 0;
}

void ext2_dir_prepare(ext2_t* ext2, uint32_t idx, uint32_t len, int type) {
  ext2_rd_ind(ext2, idx);
  if (type == TYPE_DIR) {
    ext2->ind.size = DIR_AMUT;  // maybe wrong
    ext2->ind.blocks = 1;       // "."
    ext2->ind.block[0] = ext2_alloc_block(ext2);
    ext2->dir[0].inode = idx;
    ext2->dir[1].inode = ext2->current_dir;
    ext2->dir[0].name_len = len;
    ext2->dir[1].name_len = ext2->current_dir_name_len;
    ext2->dir[0].file_type = ext2->dir[1].file_type = TYPE_DIR;
    for (int k = 2; k < DIR_AMUT; k++) ext2->dir[k].inode = 0;
    strcpy(ext2->dir[0].name, ".");
    strcpy(ext2->dir[1].name, "..");
    ext2_wr_dir(ext2, ext2->ind.block[0]);
    ext2->ind.mode = 01006; /* drwxrwxrwx: ? */
  } else {
    ext2->ind.size = 0;
    ext2->ind.blocks = 0;
    ext2->ind.mode = 00407; /* drwxrwxrwx: ? */
  }
  ext2_wr_ind(ext2, idx);
}

void ext2_remove_block(ext2_t* ext2, uint32_t del_num) {
  uint32_t tmp = del_num / 8;
  ext2_rd_blockbitmap(ext2);
  switch (del_num % 8) {
    case 0:
      ext2->blockbitmapbuf[tmp] &= 127;
      break; /* 127 = 0b 01111111 */
    case 1:
      ext2->blockbitmapbuf[tmp] &= 191;
      break; /* 191 = 0b 10111111 */
    case 2:
      ext2->blockbitmapbuf[tmp] &= 223;
      break; /* 223 = 0b 11011111 */
    case 3:
      ext2->blockbitmapbuf[tmp] &= 239;
      break; /* 239 = 0b 11101111 */
    case 4:
      ext2->blockbitmapbuf[tmp] &= 247;
      break; /* 247 = 0b 11110111 */
    case 5:
      ext2->blockbitmapbuf[tmp] &= 251;
      break; /* 251 = 0b 11111011 */
    case 6:
      ext2->blockbitmapbuf[tmp] &= 253;
      break; /* 253 = 0b 11111101 */
    case 7:
      ext2->blockbitmapbuf[tmp] &= 254;
      break; /* 254 = 0b 11111110 */
  }
  ext2_wr_blockbitmap(ext2);
  ext2->gdt.free_blocks_count++;
  ext2_wr_gd(ext2);
}

void ext2_remove_inode(ext2_t* ext2, uint32_t del_num) {
  uint32_t tmp = (del_num - 1) / 8;
  ext2_rd_inodebitmap(ext2);
  switch ((del_num - 1) % 8) {
    case 0:
      ext2->inodebitmapbuf[tmp] &= 127;
      break; /* 127 = 0b 01111111 */
    case 1:
      ext2->inodebitmapbuf[tmp] &= 191;
      break; /* 191 = 0b 10111111 */
    case 2:
      ext2->inodebitmapbuf[tmp] &= 223;
      break; /* 223 = 0b 11011111 */
    case 3:
      ext2->inodebitmapbuf[tmp] &= 239;
      break; /* 239 = 0b 11101111 */
    case 4:
      ext2->inodebitmapbuf[tmp] &= 247;
      break; /* 247 = 0b 11110111 */
    case 5:
      ext2->inodebitmapbuf[tmp] &= 251;
      break; /* 251 = 0b 11111011 */
    case 6:
      ext2->inodebitmapbuf[tmp] &= 253;
      break; /* 253 = 0b 11111101 */
    case 7:
      ext2->inodebitmapbuf[tmp] &= 254;
      break; /* 254 = 0b 11111110 */
  }
  ext2_wr_inodebitmap(ext2);
  ext2->gdt.free_inodes_count++;
  ext2_wr_gd(ext2);
}

int ext2_search_file(ext2_t* ext2, uint32_t idx) {
  for (int i = 0; i < MAX_OPEN_FILE_AMUT; i++)
    if (ext2->file_open_table[i] == idx) return 1;
  return 0;
}

void ext2_cd(ext2_t* ext2, char* dirname) {
  uint32_t i, j, k, flag;
  if (!strcmp(dirname, "../")) dirname[2] = '\0';
  if (!strcmp(dirname, "./")) dirname[1] = '\0';

  flag = ext2_reserch_file(ext2, dirname, TYPE_DIR, &i, &j, &k);
  if (flag) {
    ext2->current_dir = i;
    if (!strcmp(dirname, "..") && ext2->dir[k - 1].name_len) {
      ext2->current_dir_name[strlen(ext2->current_dir_name) -
                             ext2->dir[k - 1].name_len - 1] = '\0';
      ext2->current_dir_name_len = ext2->dir[k].name_len;
    } else if (!strcmp(dirname, "."))
      ;
    else if (strcmp(dirname, "..")) {
      ext2->current_dir_name_len = strlen(dirname);
      strcat(ext2->current_dir_name, dirname);
      strcat(ext2->current_dir_name, "/");
    }
    printf("Now in: [%s]\n", ext2->current_dir_name);
  } else {
    printf("The directory [%s] not exists!\n", dirname);
  }
}