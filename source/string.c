#include "string.h"
#include <string.h>

struct String makeString(char *s) {
  size_t length = strlen(s);
  return (struct String){.s = s, .length = length};
}
