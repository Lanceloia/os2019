#include <klib.h>

#include "../../include/fs/procfs.h"
#include "../../include/fs/vfs.h"

/* functions
 * copyright: leungjyufung2019@outlook.com
 */

proc_t procs[MAX_PROC];
int procs_size = 0;

void proc_ls(char *dirname, char *out) {
  int offset = sprintf(out, "");
  if (!strcmp(dirname, ".")) {
    offset += sprintf(out + offset, "proc            cpu      schdule times\n");
    for (int i = 0; i < procs_size; i++) {
      offset += sprintf(out + offset, "%s", procs[i].name);
      for (int j = 0; j < 15 - strlen(procs[i].name); j++)
        offset += sprintf(out + offset, "%c", ' ');
      offset += sprintf(out + offset, " %d        ", procs[i].cpu_number);
      offset += sprintf(out + offset, "%d", procs[i].schduel_times);
      offset += sprintf(out + offset, "\n");
    }
  } else {
    offset += sprintf(out + offset, "only support 'ls .'\n");
  }
}