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
