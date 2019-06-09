#include <simext2.h>
/* functions
 * copyright: leungjyufung2019@outlook.com
 */

static void ext2_init(ext2_t* fs, char* rd_name) {
  fs->rd = (rd_t*)dev_lookup(rd_name);
}

static void ext2_rd_sb(ext2_t* fs) {
  fs->ops->read(fs->rd, DISK_START, fs->sb, SB_SIZE);
}

static void ext2_wr_sb(ext2_t* fs) {
  fs->ops->write(fs->rd, DISK_START, fs->sb, SB_SIZE);
}