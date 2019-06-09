#include <simext2.h>
/* functions
 * copyright: leungjyufung2019@outlook.com
 */

static void ext2_init(ext2_t* fs, char* dev_name) {
  fs->dev = dev_lookup(dev_name);
}

static void ext2_rd_sb(ext2_t* fs) {
  fs->dev.ops->read(fs->dev, DISK_START, fs->sb, SB_SIZE);
}

static void ext2_wr_sb(ext2_t* fs) {
  fs->dev.ops->write(fs->dev, DISK_START, fs->sb, SB_SIZE);
}