#ifndef UNDO
#define UNDO

#include "zipperBuffer.h"

typedef struct UndoStack UndoStack;

struct UndoStack {
  RowList *forwards;
  RowList *backwards;
  int cursorX;
  int cursorY;
  UndoStack *tail;
};


UndoStack *undoCons(RowList *forwards,
                    RowList *backwards,
                    int cursorX,
                    int cursorY,
                    UndoStack *tail);

#endif
