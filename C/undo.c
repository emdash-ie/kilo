#include "undo.h"

UndoStack *undoCons(RowList *forwards,
                    RowList *backwards,
                    int cursorX,
                    int cursorY,
                    UndoStack *tail) {
  UndoStack *new = malloc(sizeof(*new));
  new->tail = tail;
  new->forwards = forwards;
  new->backwards = backwards;
  new->cursorX = cursorX;
  new->cursorY = cursorY;
  return new;
}
