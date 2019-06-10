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
  fs->dev->ops->read(fs->dev, GDT_START, &fs->gd, GD_SIZE);
}

void ext2_wr_gd(ext2_t* fs) {
  fs->dev->ops->read(fs->dev, GDT_START, &fs->gd, GD_SIZE);
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
  fs->dev->ops->read(fs->dev, offset, &fs->dir, DIR_SIZE);
}

void ext2_wr_dir(ext2_t* fs, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  fs->dev->ops->write(fs->dev, offset, &fs->dir, DIR_SIZE);
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