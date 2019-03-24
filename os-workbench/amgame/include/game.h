#include <am.h>
#include <amdev.h>

#define SIDE 16

int puts(const char *s) {
  for (; *s; s++)
    _putc(*s);
  return 0;
}
