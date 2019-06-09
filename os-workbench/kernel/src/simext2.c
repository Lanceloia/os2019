#include <devices.h>
#include <simext2.h>

static void simext2_init(ext2_t* fs, char* rd_name) {
  fs->rd = (rd_t*)dev_lookup(rd_name);
}

static void update_sb(ext2_t* fs) {
  fs->rd->ops->write(fs->rd, DISK_START, fs->sb, SB_SIZE);
}