#ifndef __SIMEXT2_H__
#define __SIMEXT2_H__

#include <stdint.h>
#define DISK_START (0)                // disk offset
#define GDT_START (DISK_START + 512)  // group_desc table offset
#define VOLUME_NAME "EXT2FS"          // volume name

struct super_block {
  /* super block, 32 bytes */
  char volume_name[16];
  uint16_t disk_size;  // maybe have no use
  uint16_t blocks_per_group;
  uint16_t inodes_per_blocks;
  char pad[10];
};

struct group_desc {
  /* block group descriptor, 32 bytes */
  char volume_name[16];
  uint16_t block_bitmap;       // the blk idx of data blk-bitmap
  uint16_t inode_bitmap;       // the blk idx of inode-bitmap
  uint16_t inode_table;        // the blk idx of inode-table
  uint16_t free_blocks_count;  // the amount of free blks
  uint16_t free_inodes_count;  // the amount of free inodes
  uint16_t used_dirs_count;    // the amount of dirs
  char pad[4];
};

struct real_inode {
  /* real inode, 64 bytes */
  uint16_t mode;      // the mode of file
  uint16_t blocks;    // the size of file (blks)
  uint32_t size;      // the size of file (bytes)
  uint16_t block[8];  // direct or indirect blocks
  char pad[40];
};

struct ext2fs {
  struct super_block sb;
  struct group_desc gdt;
};
#endif
