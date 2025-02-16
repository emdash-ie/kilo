#include <stddef.h>

struct String {
  char *s;
  size_t length;
};

struct String makeString(char *s);
