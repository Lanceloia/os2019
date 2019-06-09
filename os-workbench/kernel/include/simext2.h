
#ifndef __SIMEXT2_H__
#define __SIMEXT2_H__

#include "devices.h"

#define SB_SIZE (sizeof(sb_t))
#define GD_SIZE (sizeof(gd_t))
#define IND_SIZE (sizeof(ind_t))
#define DISK_START (0)                // disk offset
#define GDT_START (DISK_START + 512)  // group_desc table offset
#define EXT2_N_BLOCKS (15)            // ext2 inode blocks
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

struct inode {
  /* inode, 32 bytes */
  uint16_t mode;                  // the mode of file
  uint16_t uid;                   // the uid of owner
  uint32_t blocks;                // the size of file (blks)
  uint32_t size;                  // the size of file (bytes)
  uint32_t block[EXT2_N_BLOCKS];  // direct or indirect blocks
  char pad[14];
};

struct dir_entry {
  /* directory entry, 32 bytes */
  uint32_t inode;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
  char name[16];
  char pad[26];
};

struct ext2 {
  struct super_block sb[1];
  struct group_desc gdt[1];
  struct inode inode_area[1];
  devices_t* dev;
};

typedef struct super_block sb_t;
typedef struct group_desc gd_t;
typedef struct inode ind_t;
typedef struct ext2 ext2_t;

#endif