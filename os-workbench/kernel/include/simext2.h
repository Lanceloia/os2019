#include <unistd.h>
#define INODE_SIZE 64
#define DISK_START 0
#define N_BLOCKS 8

struct super_block {
  /* super block 32 bytes */
  char volume_name[16];
  uint16_t disk_size;  // maybe have no use
  uint16_t blocks_per_group;
  uint16_t inodes_per_blocks;
  char pad[10];
};

struct group_desc {
  /* block group descriptor 32 bytes */
  char volume_name[16];
  uint16_t block_bitmap;       // the blk idx of data blk-bitmap
  uint16_t inode_bitmap;       // the blk idx of inode-bitmap
  uint16_t inode_table;        // the blk idx of inode-table
  uint16_t free_blocks_count;  // the amount of free blks
  uint16_t free_inodes_count;  // the amount of free inodes
  uint16_t used_dirs_count;    // the amount of dirs
  char pad[4];
};

struct inode {
  uint16_t mode;       // the mode of file
  uint16_t blocks;     // the size of file (blks)
  uint32_t size;       // the size of file (bytes)
  uint32_t ref_count;  // the f
  void *ptr;           // private data start
  filesystem_t *fs;
  inodeops_t *ops;
  uint32_t block[N_BLOCKS]
};
