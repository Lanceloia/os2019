#include <klib.h>

#include "../../include/fs/procfs.h"
#include "../../include/fs/vfs.h"

/* functions
 * copyright: leungjyufung2019@outlook.com
 */

proc_t procs[MAX_PROC];

void proc_ls(char *dirname, char *pwd, char *out) {
  int offset = sprintf(out, ""), tmp_dir = -1;
  if (!strcmp(dirname, "."))
    tmp_dir = vfsdirs[cur_dir].dot;
  else if (!strcmp(dirname, ".."))
    tmp_dir = vfsdirs[cur_dir].ddot;
  else {
    for (int i = vfsdirs[cur_dir].child; i != -1; i = vfsdirs[i].next) {
      if (!strcmp(dirname, vfsdirs[i].name)) tmp_dir = i, i = -1;
    }
  }
  if (tmp_dir == -1)
    offset += sprintf(out + offset, "No such directory.\n");
  else {
    offset += sprintf(out + offset, "items           type     path\n");
    for (int i = vfsdirs[tmp_dir].child; i != -1; i = vfsdirs[i].next) {
      offset += sprintf(out + offset, "%s", vfsdirs[i].name);
      for (int j = 0; j < 15 - strlen(vfsdirs[i].name); j++)
        offset += sprintf(out + offset, "%c", ' ');
      offset += sprintf(out + offset, " <DIR>    ");
      offset += sprintf(out + offset, "%s", vfsdirs[i].absolutely_name);
      offset += sprintf(out + offset, "\n");
    }
  }
}