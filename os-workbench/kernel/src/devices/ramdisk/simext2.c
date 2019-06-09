#include <simext2.h>
/* functions
 * copyright: leungjyufung2019@outlook.com
 */

static void ext2_init(ext2_t* fs, char* rd_name) {
  fs->dev = dev_lookup(rd_name);
  fs->ops;
}

static void ext2_rd_sb(ext2_t* fs) {
  fs->ops->read(dev->rd, DISK_START, fs->sb, SB_SIZE);
}

static void ext2_wr_sb(ext2_t* fs) {
  fs->ops->write(dev->rd, DISK_START, fs->sb, SB_SIZE);
}