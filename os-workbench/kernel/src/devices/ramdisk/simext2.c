#include <simext2.h>

/* functions
 * copyright: leungjyufung2019@outlook.com
 */

void ext2_init(ext2_t* fs, char* dev_name) { fs->dev = dev_lookup(dev_name); }

void ext2_rd_sb(ext2_t* fs) {
  fs->dev->ops->read(fs->dev, DISK_START, &fs->sb, SB_SIZE);
}

void ext2_wr_sb(ext2_t* fs) {
  fs->dev->ops->write(fs->dev, DISK_START, &fs->sb, SB_SIZE);
}

void ext2_rd_gd(ext2_t* fs) {
  fs->dev->ops->read(fs->dev, GDT_START, &fs->gdt, GD_SIZE);
}

void ext2_wr_gd(ext2_t* fs) {
  fs->dev->ops->read(fs->dev, GDT_START, &fs->gdt, GD_SIZE);
}

void ext2_rd_ind(ext2_t* fs, uint32_t i) {
  uint32_t offset = INDT_START + (i - 1) * IND_SIZE;
  fs->dev->ops->read(fs->dev, offset, &fs->ind, IND_SIZE);
}

void ext2_wr_ind(ext2_t* fs, uint32_t i) {
  uint32_t offset = INDT_START + (i - 1) * IND_SIZE;
  fs->dev->ops->write(fs->dev, offset, &fs->ind, IND_SIZE);
}

void ext2_rd_dir(ext2_t* fs, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  fs->dev->ops->read(fs->dev, offset, &fs->dir, BLK_SIZE);
}

void ext2_wr_dir(ext2_t* fs, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  fs->dev->ops->write(fs->dev, offset, &fs->dir, BLK_SIZE);
}

void ext2_rd_blockbitmap(ext2_t* fs) {
  fs->dev->ops->read(fs->dev, BLK_BITMAP, &fs->blockbitmapbuf, BLK_SIZE);
}

void ext2_wr_blockbitmap(ext2_t* fs) {
  fs->dev->ops->write(fs->dev, BLK_BITMAP, &fs->blockbitmapbuf, BLK_SIZE);
}

void ext2_rd_inodebitmap(ext2_t* fs) {
  fs->dev->ops->read(fs->dev, IND_BITMAP, &fs->inodebitmapbuf, BLK_SIZE);
}

void ext2_wr_inodebitmap(ext2_t* fs) {
  fs->dev->ops->write(fs->dev, IND_BITMAP, &fs->inodebitmapbuf, BLK_SIZE);
}

void ext2_rd_datablock(ext2_t* fs, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  fs->dev->ops->read(fs->dev, offset, &fs->datablockbuf, BLK_SIZE);
}

void ext2_wr_datablock(ext2_t* fs, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  fs->dev->ops->write(fs->dev, offset, &fs->datablockbuf, BLK_SIZE);
}

uint32_t ext2_alloc_block(ext2_t* fs) {
  uint32_t cur = fs->last_alloc_block / 8;
  uint32_t con = 0x80; /* 0b10000000 */
  int flag = 0;
  if (fs->gdt.free_blocks_count == 0) {
    return -1;
  }
  ext2_rd_blockbitmap(fs);
  while (fs->blockbitmapbuf[cur] == 0xff) {
    if (cur == BLK_SIZE - 1)
      cur = 0;  // restart from zero
    else
      cur++;  // try next
  }
  while (fs->blockbitmapbuf[cur] & con) {
    con = con / 2;
    flag++;
  }
  fs->blockbitmapbuf[cur] = fs->blockbitmapbuf[cur] + con;
  fs->last_alloc_block = cur * 8 + flag;
  ext2_wr_blockbitmap(fs);
  fs->gdt.free_blocks_count--;
  ext2_wr_gd(fs);
  return fs->last_alloc_block;
}

uint32_t ext2_alloc_inode(ext2_t* fs) {
  uint32_t cur = (fs->last_alloc_inode - 1) / 8;
  uint32_t con = 0x80; /* 0b10000000 */
  int flag = 0;
  if (fs->gdt.free_inodes_count == 0) {
    return -1;
  }
  ext2_rd_inodebitmap(fs);
  while (fs->inodebitmapbuf[cur] == 0xff) {
    if (cur == BLK_SIZE - 1)
      cur = 0;  // restart from zero
    else
      cur++;  // try next
  }
  while (fs->inodebitmapbuf[cur] & con) {
    con = con / 2;
    flag++;
  }
  fs->inodebitmapbuf[cur] = fs->inodebitmapbuf[cur] + con;
  fs->last_alloc_inode = cur * 8 + flag + 1;  // why add 1 here?
  ext2_wr_inodebitmap(fs);
  fs->gdt.free_inodes_count--;
  ext2_wr_gd(fs);
  return fs->last_alloc_inode;
}

uint32_t ext2_reserch_file(ext2_t* fs, char* name, int file_type,
                           uint32_t* inode_num, uint32_t* block_num,
                           uint32_t* dir_num) {
  ext2_rd_ind(fs, fs->current_dir);
  for (uint32_t j = 0; j < fs->ind.blocks; j++) {
    ext2_rd_dir(fs, fs->ind.block[j]);
    for (uint32_t k = 0; k < 32;) {
      if (!fs->dir[k].inode || fs->dir[k].file_type != file_type ||
          strcmp(dir[k].name, name)) {
        k++;
      } else {
        *inode_num = fs->dir[k].inode;
        *block_num = j;
        *dir_num = k;
        return 1;
      }
    }
  }
  return 0;
}

void ext2_dir_prepare(ext2_t* fs, uint32_t idx, uint32_t len, int type) {
  ext2_rd_ind(fs, idx);
  if (type == TYPE_DIR) {
    fs->ind.size = DIR_AMUT;  // maybe wrong
    fs->ind.blocks = 1;       // "."
    fs->ind.block[0] = ext2_alloc_block(fs);
    fs->dir[0].inode = idx;
    fs->dir[1].inode = fs->current_dir;
    fs->dir[0].name_len = len;
    fs->dir[1].name_len = fs->current_dir_name_len;
    fs->dir[0].file_type = fs->dir[1].file_type = TYPE_DIR;
    for (int k = 2; k < DIR_AMUT; k++) fs->dir[k].inode = 0;
    strcpy(fs->dir[0].name, ".");
    strcpy(fs->dir[1].name, "..");
    ext2_wr_dir(fs, fs->ind.block[0]);
    fs->ind.mode = 01006 /* drwxrwxrwx: ? */
  } else {
    fs->ind.size = 0;
    fs->ind.blocks = 0;
    fs->ind.mode = 00407 /* drwxrwxrwx: ? */
  }
  ext2_wr_ind(fs, idx);
}