int main() {
#pragma once

#include <stdlib.h>

#define _Tuple(a, b) Tuple__ ## a ## b
#define Tuple(a, b) _Tuple(a, b)

#define DefineTuple(a, b) \
  DefineTupleStruct(a, b)

#define DefineTupleStruct(a, b) \
  typedef struct Tuple(a, b) { \
    a *fst; \
    b *snd; \
  } Tuple(a, b);
return 0;
}
