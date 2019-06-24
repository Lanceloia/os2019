#include <klib.h>

#include "../../include/fs/ext2fs.h"
#include "../../include/fs/vfs.h"

/* functions
 * copyright: leungjyufung2019@outlook.com
 */
uint32_t ext2_alloc_block(ext2_t* ext2);
uint32_t ext2_alloc_inode(ext2_t* ext2);
uint32_t ext2_reserch_file(ext2_t* ext2, char* name, int mode,
                           uint32_t* inode_num, uint32_t* block_num,
                           uint32_t* dir_num);
void ext2_ind_prepare(ext2_t* ext2, uint32_t idx, uint32_t par, int mode);
void ext2_remove_block(ext2_t* ext2, uint32_t del_num);
int ext2_search_file(ext2_t* ext2, uint32_t idx);

int ext2_create(ext2_t* ext2, int ridx, char* name, int mode);
int ext2_remove(ext2_t* ext2, int ridx, char* name, int mode);
void ext2_cd(ext2_t* ext2, char* dirname);
// ssize_t ext2_read(ext2_t*, int, char*, uint32_t);
ssize_t ext2_write(ext2_t*, int, uint64_t, char*, uint32_t);

static int first_item_len(const char* path) {
  int ret = 0;
  for (; path[ret] != '\0' && path[ret] != '/';) ret++;
  return ret;
}

/*
static int last_item_offset(const char* path) {
  int offset = 0, ret = 0;
  for (; path[offset] != '\0'; offset++)
    if (path[offset] == '/') ret = offset + 1;
  return ret;
}
*/

#define ouput(str, ...) offset += sprintf(out + offset, str, ...)

char* hello_str =
    "#include <iostream> \nusing namespace std;\nint main(){\n  return 0;\n}\n";
char trash[4096];

int ext2_init(filesystem_t* fs, const char* name, device_t* dev) {
  ext2_t* ext2 = (ext2_t*)fs->rfs;
  memset(ext2, 0x00, sizeof(ext2_t));
  ext2->dev = dev;
  printf("Creating ext2: %s\n", name);
  ext2->last_alloc_inode = 1;
  ext2->last_alloc_block = 0;
  for (int i = 0; i < MAX_OPEN_FILE_AMUT; i++) ext2->file_open_table[i] = 0;
  ext2_rd_ind(ext2, 1);
  ext2_rd_dir(ext2, 0);
  ext2_rd_gd(ext2);  // gdt is changed here
  ext2->gdt.block_bitmap = BLK_BITMAP;
  ext2->gdt.inode_bitmap = IND_BITMAP;
  ext2->gdt.inode_table = INDT_START;  // maye be no use
  ext2->gdt.free_blocks_count = DATA_BLOCK_COUNT;
  ext2->gdt.free_inodes_count = INODE_TABLE_COUNT;
  ext2->gdt.used_dirs_count = 0;
  ext2_wr_gd(ext2);

  ext2_rd_blockbitmap(ext2);
  ext2_rd_inodebitmap(ext2);
  ext2->ind.mode = TYPE_DIR | RD_ABLE | WR_ABLE;
  ext2->ind.blocks = 0;
  ext2->ind.size = 2 * DIR_SIZE;  // origin 32, maybe wrong
  ext2->ind.block[0] = ext2_alloc_block(ext2);
  ext2->ind.blocks++;

  ext2->current_dir = ext2_alloc_inode(ext2);
  // printf("cur_dir: %d\n", ext2->current_dir);
  ext2_wr_ind(ext2, ext2->current_dir);
  // "." == ".." == root_dir
  // root_dir with no name
  ext2->dir[0].inode = ext2->dir[1].inode = ext2->current_dir;
  ext2->dir[0].name_len = ext2->dir[1].name_len = 0;
  strcpy(ext2->dir[0].name, ".");
  strcpy(ext2->dir[1].name, "..");
  ext2_wr_dir(ext2, ext2->ind.block[0]);

  /* test */
  int hello_cpp = ext2_create(ext2, ext2->current_dir, "hello.cpp", TYPE_FILE);
  ext2_write(ext2, hello_cpp, 0, hello_str, strlen(hello_str));
  ext2_create(ext2, ext2->current_dir, "default_dir", TYPE_DIR);
  return 1;
}

int ext2_lookup(filesystem_t* fs, const char* path, int mode) { return 0; }
int ext2_readdir(filesystem_t* fs, int ridx, int kth, vinode_t* buf) {
  // printf("fuck: kth == %d\n", kth);
  ext2_t* ext2 = (ext2_t*)fs->rfs;
  int cnt = 0;
  ext2_rd_ind(ext2, ridx);
  // printf("rinode: %d, kth: %d\n", ridx, kth);
  for (int i = 0; i < ext2->ind.blocks; i++) {
    ext2_rd_dir(ext2, ext2->ind.block[i]);
    for (int k = 0; k < DIR_AMUT; k++) {
      if (ext2->dir[k].inode)
        if (++cnt == kth) {
          strcpy(buf->name, ext2->dir[k].name);
          buf->ridx = ext2->dir[k].inode;
          buf->mode = ext2->dir[k].mode;
          return 1;
        }
    }
  }
  return 0;
}

uint32_t ext2_alloc_block(ext2_t* ext2) {
  uint32_t cur = ext2->last_alloc_block / 8;
  uint32_t con = 0x80; /* 0b10000000 */
  int flag = 0;
  if (ext2->gdt.free_blocks_count == 0) {
    return -1;
  }
  ext2_rd_blockbitmap(ext2);
  while (ext2->blockbitmapbuf[cur] == 0xff) {
    if (cur == BLK_SIZE - 1)
      cur = 0;  // restart from zero
    else
      cur++;  // try next
  }
  while (ext2->blockbitmapbuf[cur] & con) {
    con = con / 2;
    flag++;
  }
  ext2->blockbitmapbuf[cur] = ext2->blockbitmapbuf[cur] + con;
  ext2->last_alloc_block = cur * 8 + flag;
  ext2_wr_blockbitmap(ext2);
  ext2->gdt.free_blocks_count--;
  ext2_wr_gd(ext2);
  return ext2->last_alloc_block;
}

uint32_t ext2_alloc_inode(ext2_t* ext2) {
  uint32_t cur = (ext2->last_alloc_inode - 1) / 8;
  uint32_t con = 0x80; /* 0b10000000 */
  int flag = 0;
  if (ext2->gdt.free_inodes_count == 0) {
    return -1;
  }
  ext2_rd_inodebitmap(ext2);
  while (ext2->inodebitmapbuf[cur] == 0xff) {
    if (cur == BLK_SIZE - 1)
      cur = 0;  // restart from zero
    else
      cur++;  // try next
  }
  while (ext2->inodebitmapbuf[cur] & con) {
    con = con / 2;
    flag++;
  }
  ext2->inodebitmapbuf[cur] = ext2->inodebitmapbuf[cur] + con;
  ext2->last_alloc_inode = cur * 8 + flag + 1;
  ext2_wr_inodebitmap(ext2);
  ext2->gdt.free_inodes_count--;
  ext2_wr_gd(ext2);
  return ext2->last_alloc_inode;
}

uint32_t ext2_reserch_file(ext2_t* ext2, char* path, int mode, uint32_t* ninode,
                           uint32_t* nblock, uint32_t* ndir) {
  ext2_rd_ind(ext2, ext2->current_dir);
  for (uint32_t j = 0; j < ext2->ind.blocks; j++) {
    ext2_rd_dir(ext2, ext2->ind.block[j]);
    int len = first_item_len(path);
    for (uint32_t k = 0; k < DIR_AMUT;) {
      if (!ext2->dir[k].inode || !(ext2->dir[k].mode & mode) ||
          strncmp(ext2->dir[k].name, path, len)) {
        k++;
      } else {
        ext2->current_dir = *ninode = ext2->dir[k].inode;
        *nblock = j;
        *ndir = k;
        return (len == strlen(path))
                   ? 1
                   : ext2_reserch_file(ext2, path + len + 1, mode, ninode,
                                       nblock, ndir);
      }
    }
  }
  return 0;
}

void ext2_ind_prepare(ext2_t* ext2, uint32_t idx, uint32_t par, int mode) {
  ext2_rd_ind(ext2, idx);
  if (mode == TYPE_DIR) {
    ext2->ind.size = 2 * DIR_SIZE;  // "." and ".."
    ext2->ind.blocks = 1;
    ext2->ind.block[0] = ext2_alloc_block(ext2);
    ext2->dir[0].inode = idx;
    ext2->dir[1].inode = par;
    ext2->dir[0].mode = ext2->dir[1].mode = TYPE_DIR;
    for (int k = 2; k < DIR_AMUT; k++) ext2->dir[k].inode = 0;
    strcpy(ext2->dir[0].name, ".");
    strcpy(ext2->dir[1].name, "..");
    ext2_wr_dir(ext2, ext2->ind.block[0]);
    ext2->ind.mode = TYPE_DIR;
  } else {
    ext2->ind.size = 0;
    ext2->ind.blocks = 0;
    ext2->ind.mode = TYPE_FILE;
  }
  ext2_wr_ind(ext2, idx);
}

void ext2_remove_block(ext2_t* ext2, uint32_t del_num) {
  uint32_t tmp = del_num / 8;
  ext2_rd_blockbitmap(ext2);
  switch (del_num % 8) {
    case 0:
      ext2->blockbitmapbuf[tmp] &= 127;
      break; /* 127 = 0b 01111111 */
    case 1:
      ext2->blockbitmapbuf[tmp] &= 191;
      break; /* 191 = 0b 10111111 */
    case 2:
      ext2->blockbitmapbuf[tmp] &= 223;
      break; /* 223 = 0b 11011111 */
    case 3:
      ext2->blockbitmapbuf[tmp] &= 239;
      break; /* 239 = 0b 11101111 */
    case 4:
      ext2->blockbitmapbuf[tmp] &= 247;
      break; /* 247 = 0b 11110111 */
    case 5:
      ext2->blockbitmapbuf[tmp] &= 251;
      break; /* 251 = 0b 11111011 */
    case 6:
      ext2->blockbitmapbuf[tmp] &= 253;
      break; /* 253 = 0b 11111101 */
    case 7:
      ext2->blockbitmapbuf[tmp] &= 254;
      break; /* 254 = 0b 11111110 */
  }
  ext2_wr_blockbitmap(ext2);
  ext2->gdt.free_blocks_count++;
  ext2_wr_gd(ext2);
}

void ext2_remove_inode(ext2_t* ext2, uint32_t del_num) {
  uint32_t tmp = (del_num - 1) / 8;
  ext2_rd_inodebitmap(ext2);
  switch ((del_num - 1) % 8) {
    case 0:
      ext2->inodebitmapbuf[tmp] &= 127;
      break; /* 127 = 0b 01111111 */
    case 1:
      ext2->inodebitmapbuf[tmp] &= 191;
      break; /* 191 = 0b 10111111 */
    case 2:
      ext2->inodebitmapbuf[tmp] &= 223;
      break; /* 223 = 0b 11011111 */
    case 3:
      ext2->inodebitmapbuf[tmp] &= 239;
      break; /* 239 = 0b 11101111 */
    case 4:
      ext2->inodebitmapbuf[tmp] &= 247;
      break; /* 247 = 0b 11110111 */
    case 5:
      ext2->inodebitmapbuf[tmp] &= 251;
      break; /* 251 = 0b 11111011 */
    case 6:
      ext2->inodebitmapbuf[tmp] &= 253;
      break; /* 253 = 0b 11111101 */
    case 7:
      ext2->inodebitmapbuf[tmp] &= 254;
      break; /* 254 = 0b 11111110 */
  }
  ext2_wr_inodebitmap(ext2);
  ext2->gdt.free_inodes_count++;
  ext2_wr_gd(ext2);
}

int ext2_search_file(ext2_t* ext2, uint32_t idx) {
  for (int i = 0; i < MAX_OPEN_FILE_AMUT; i++)
    if (ext2->file_open_table[i] == idx) return 1;
  return 0;
}

void ext2_cd(ext2_t* ext2, char* dirname) {
  uint32_t i, j, k, flag;
  if (!strcmp(dirname, "../")) dirname[2] = '\0';
  if (!strcmp(dirname, "./")) dirname[1] = '\0';
  flag = ext2_reserch_file(ext2, dirname, TYPE_DIR, &i, &j, &k);
  if (flag)
    ext2->current_dir = i;
  else
    printf("No directory: %s\n", dirname);
}

ssize_t ext2_read(ext2_t* ext2, int ridx, uint64_t offset, char* buf,
                  uint32_t len) {
  int skip_blocks = offset / BLK_SIZE;
  int first_offset = offset - skip_blocks * BLK_SIZE;

  ext2_rd_ind(ext2, ridx);
  int ret = 0;
  for (int i = skip_blocks; i < ext2->ind.blocks; i++) {
    ext2_rd_datablock(ext2, ext2->ind.block[i]);
    if (i == skip_blocks)
      for (int j = 0; j < ext2->ind.size - i * BLK_SIZE; ++j) {
        if (ret == len || ret + offset == ext2->ind.size) return ret;
        ret += sprintf(buf + ret, "%c", ext2->datablockbuf[j + first_offset]);
      }
    else
      for (int j = 0; j < ext2->ind.size - i * BLK_SIZE; ++j) {
        if (ret == len || ret + offset == ext2->ind.size) return ret;
        ret += sprintf(buf + ret, "%c", ext2->datablockbuf[j]);
      }
  }
  return ret;
}

ssize_t ext2_write(ext2_t* ext2, int ridx, uint64_t offset, char* buf,
                   uint32_t len) {
  int skip_blocks = offset / BLK_SIZE;
  int first_offset = offset - skip_blocks * BLK_SIZE;

  int need_blocks = (len + offset + (BLK_SIZE - 1)) / BLK_SIZE;

  ssize_t ret = 0;
  ext2_rd_ind(ext2, ridx);
  if ((ext2->ind.mode & WR_ABLE) == 0) {
    printf("File can't be writed!\n");
    return 0;
  }

  if (ext2->ind.blocks <= need_blocks) {
    while (ext2->ind.blocks < need_blocks)
      ext2->ind.block[ext2->ind.blocks++] = ext2_alloc_block(ext2);
  } else {
    while (ext2->ind.blocks > need_blocks)
      ext2_remove_block(ext2, ext2->ind.block[--ext2->ind.blocks]);
  }

  for (int n = skip_blocks; n < need_blocks; n++) {
    if (n == skip_blocks) {
      ext2_rd_datablock(ext2, ext2->ind.block[n]);
      for (int k = first_offset; ret < len && k < BLK_SIZE; k++, ret++)
        ext2->datablockbuf[k] = buf[ret];
      ext2_wr_datablock(ext2, ext2->ind.block[n]);
    } else if (n != need_blocks - 1) {
      ext2_rd_datablock(ext2, ext2->ind.block[n]);
      for (int k = 0; ret < len && k < BLK_SIZE; k++, ret++)
        ext2->datablockbuf[k] = buf[ret];
      ext2_wr_datablock(ext2, ext2->ind.block[n]);
    } else {
      ext2_rd_datablock(ext2, ext2->ind.block[n]);
      for (int k = 0; k < len - ret; k++, ret++)
        ext2->datablockbuf[k] = buf[ret];
      ext2_wr_datablock(ext2, ext2->ind.block[n]);
    }
  }

  if (ret != len) {
    printf("ret == %d, len == %d\n", ret, len);
  }
  ext2->ind.size = offset + len;
  ext2_wr_ind(ext2, ridx);

  return ret;
}

int ext2_create(ext2_t* ext2, int ridx, char* name, int mode) {
  ext2_rd_ind(ext2, ridx);

  assert(ext2->ind.size < 4096);
  int idx;
  if (ext2->ind.size != ext2->ind.blocks * BLK_SIZE) {
    int i, j;
    for (i = 0; i < ext2->ind.blocks; i++) {
      ext2_rd_dir(ext2, ext2->ind.block[i]);
      for (j = 0; j < DIR_AMUT; j++)
        if (ext2->dir[j].inode == 0) goto CreateEnd;
    }
  CreateEnd:
    idx = ext2->dir[j].inode = ext2_alloc_inode(ext2);
    ext2->dir[j].mode = mode;
    ext2->dir[j].name_len = strlen(name);
    strcpy(ext2->dir[j].name, name);
    ext2_wr_dir(ext2, ext2->ind.block[i]);
  } else {
    ext2->ind.block[ext2->ind.blocks++] = ext2_alloc_block(ext2);
    ext2_rd_dir(ext2, ext2->ind.block[ext2->ind.blocks - 1]);
    idx = ext2->dir[0].inode = ext2_alloc_inode(ext2);
    ext2->dir[0].mode = mode;
    ext2->dir[0].name_len = strlen(name);
    strcpy(ext2->dir[0].name, name);
    for (int i = 1; i < DIR_AMUT; i++) ext2->dir[i].inode = 0;
    ext2_wr_dir(ext2, ext2->ind.block[ext2->ind.blocks - 1]);
  }
  ext2->ind.size += DIR_SIZE;  // origin 16
  ext2_wr_ind(ext2, ridx);
  ext2_ind_prepare(ext2, idx, ridx, mode);
  return idx;
}

int ext2_remove(ext2_t* ext2, int ridx, char* name, int mode) {
  ext2_rd_ind(ext2, ridx);

  int i, j;
  for (i = 0; i < ext2->ind.blocks; i++) {
    ext2_rd_dir(ext2, ext2->ind.block[i]);
    for (j = 0; j < DIR_AMUT; j++)
      if (!strcmp(ext2->dir[j].name, name)) goto RemoveEnd;
  }
RemoveEnd:
  if (mode == TYPE_DIR) {
    ext2_rd_ind(ext2, ext2->dir[j].inode);
    if (ext2->ind.size == 2 * DIR_SIZE) {
      ext2->ind.size = ext2->ind.blocks = 0;
      ext2_remove_block(ext2, ext2->ind.block[0]);
      ext2_remove_block(ext2, ext2->ind.block[1]);
      ext2_wr_ind(ext2, ext2->dir[j].inode);

      ext2_remove_block(ext2, ext2->dir[j].inode);
      ext2->dir[j].inode = 0;
      ext2_wr_dir(ext2, ext2->dir[j].inode);
    } else {
      printf("Dir is not empty! \n");
      return 1;
    }
  } else {
    assert(0);
    return 1;
  }

  ext2_rd_ind(ext2, ridx);
  ext2->ind.size -= DIR_SIZE;
  ext2_wr_ind(ext2, ridx);

  return 0;
}

void ext2_rd_sb(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, DISK_START, &ext2->sb, SB_SIZE);
}

void ext2_wr_sb(ext2_t* ext2) {
  ext2->dev->ops->write(ext2->dev, DISK_START, &ext2->sb, SB_SIZE);
}

void ext2_rd_gd(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, GDT_START, &ext2->gdt, GD_SIZE);
}

void ext2_wr_gd(ext2_t* ext2) {
  ext2->dev->ops->write(ext2->dev, GDT_START, &ext2->gdt, GD_SIZE);
}

void ext2_rd_ind(ext2_t* ext2, uint32_t i) {
  uint32_t offset = INDT_START + (i - 1) * IND_SIZE;
  ext2->dev->ops->read(ext2->dev, offset, &ext2->ind, IND_SIZE);
}

void ext2_wr_ind(ext2_t* ext2, uint32_t i) {
  uint32_t offset = INDT_START + (i - 1) * IND_SIZE;
  ext2->dev->ops->write(ext2->dev, offset, &ext2->ind, IND_SIZE);
}

void ext2_rd_dir(ext2_t* ext2, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  ext2->dev->ops->read(ext2->dev, offset, &ext2->dir, BLK_SIZE);
}

void ext2_wr_dir(ext2_t* ext2, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  ext2->dev->ops->write(ext2->dev, offset, &ext2->dir, BLK_SIZE);
}

void ext2_rd_blockbitmap(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, BLK_BITMAP, &ext2->blockbitmapbuf, BLK_SIZE);
}

void ext2_wr_blockbitmap(ext2_t* ext2) {
  ext2->dev->ops->write(ext2->dev, BLK_BITMAP, &ext2->blockbitmapbuf, BLK_SIZE);
}

void ext2_rd_inodebitmap(ext2_t* ext2) {
  ext2->dev->ops->read(ext2->dev, IND_BITMAP, &ext2->inodebitmapbuf, BLK_SIZE);
}

void ext2_wr_inodebitmap(ext2_t* ext2) {
  ext2->dev->ops->write(ext2->dev, IND_BITMAP, &ext2->inodebitmapbuf, BLK_SIZE);
}

void ext2_rd_datablock(ext2_t* ext2, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  ext2->dev->ops->read(ext2->dev, offset, &ext2->datablockbuf, BLK_SIZE);
}

void ext2_wr_datablock(ext2_t* ext2, uint32_t i) {
  uint32_t offset = DATA_BLOCK + i * BLK_SIZE;
  ext2->dev->ops->write(ext2->dev, offset, &ext2->datablockbuf, BLK_SIZE);
}
