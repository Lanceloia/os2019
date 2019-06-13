#ifndef __EXT2FS_H__
#define __EXT2FS_H__

#include <devices.h>

#define BLK_SIZE (512)
#define DISK_START (0 * BLK_SIZE)          // disk offset
#define GDT_START (1 * BLK_SIZE)           // group_desc table offset
#define BLK_BITMAP (2 * BLK_SIZE)          // block bitmap offset
#define IND_BITMAP (3 * BLK_SIZE)          // inode bitmap offset
#define INDT_START (4 * BLK_SIZE)          // inode table offset
#define DATA_BLOCK ((4 + 512) * BLK_SIZE)  // data block offset
#define DATA_SIZE (4096)                   // data block size(blocks)
#define DISK_SIZE (DATA_SIZE + 512)        // disk size(blocks)
#define EXT2_N_BLOCKS (12)                 // ext2 inode blocks
#define VOLUME_NAME "EXT2FS"               // volume name

struct super_block {
  /* super block, 32 bytes */
  char volume_name[16];
  uint16_t disk_size;  // maybe have no use
  uint16_t blocks_per_group;
  uint16_t inodes_per_blocks;
  char pad[10];
};

#define INODE_TABLE_COUNT 4096
#define DATA_BLOCK_COUNT 4096

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
  /* inode, 64 bytes */
  uint32_t mode;                  // the mode of file
  uint32_t blocks;                // the size of file (blks)
  uint32_t size;                  // the size of file (bytes)
  uint32_t block[EXT2_N_BLOCKS];  // direct or indirect blocks
  char pad[2];
};

struct directory {
  /* directory entry, 32 bytes */
  uint32_t inode;
  uint32_t mode;
  uint16_t rec_len;
  char name[16];
  uint8_t name_len;
  char pad[5];
};

typedef struct super_block sb_t;
typedef struct group_desc gd_t;
typedef struct inode ind_t;
typedef struct directory dir_t;

#define SB_SIZE (sizeof(sb_t))
#define GD_SIZE (sizeof(gd_t))
#define IND_SIZE (sizeof(ind_t))
#define DIR_SIZE (sizeof(dir_t))
#define DIR_AMUT (BLK_SIZE / DIR_SIZE)
#define MAX_OPEN_FILE_AMUT (16)

struct ext2 {
  struct super_block sb;
  struct group_desc gdt;
  struct inode ind;
  struct directory dir[DIR_AMUT];
  unsigned char blockbitmapbuf[BLK_SIZE];
  unsigned char inodebitmapbuf[BLK_SIZE];
  unsigned char datablockbuf[BLK_SIZE];
  uint32_t last_alloc_block;
  uint32_t last_alloc_inode;
  uint32_t current_dir;
  uint32_t current_dir_name_len;
  uint32_t file_open_table[MAX_OPEN_FILE_AMUT];
  char current_dir_name[256];
  device_t* dev;
};

typedef struct ext2 ext2_t;

void ext2_rd_sb(ext2_t* ext2);
void ext2_wr_sb(ext2_t* ext2);
void ext2_rd_gd(ext2_t* ext2);
void ext2_wr_gd(ext2_t* ext2);
void ext2_rd_ind(ext2_t* ext2, uint32_t i);
void ext2_wr_ind(ext2_t* ext2, uint32_t i);
void ext2_rd_dir(ext2_t* ext2, uint32_t i);
void ext2_wr_dir(ext2_t* ext2, uint32_t i);
void ext2_rd_blockbitmap(ext2_t* ext2);
void ext2_wr_blockbitmap(ext2_t* ext2);
void ext2_rd_inodebitmap(ext2_t* ext2);
void ext2_wr_inodebitmap(ext2_t* ext2);
void ext2_rd_datablock(ext2_t* ext2, uint32_t i);
void ext2_wr_datablock(ext2_t* ext2, uint32_t i);

void ext2_ls(ext2_t* ext2, char* dirname, char* out);

#endif