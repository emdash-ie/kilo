#include "editorRow.h"
#include "lists/PaneRow.h"
#include "pane.h"
#include "util.h"
#include "zipperBuffer.h"

Pane *makePane(int cursorX, int cursorY, int top, int left, FileData *file) {
  Pane *p = malloc(sizeof(Pane));
  p->cursorX = cursorX;
  p->cursorY = cursorY;
  p->top = top;
  p->left = left;
  p->file = file;
  return p;
}

List(PaneRow) *drawRow(int left, int width, EditorRow *r) {
  int x = clip(left, 0, r->renderSize);
  int resultWidth = clip(r->renderSize - x, 0, width);
  unsigned int blanks = width - resultWidth;
  return ListF(PaneRow).cons(makePaneRow(r->renderChars + x, resultWidth, blanks), NULL);
}

List(PaneRow) *drawPane(int height, int left, int width, RowList *rows) {
  if (rows == NULL || height <= 0) {
    return NULL;
  }
  List(PaneRow) *head = drawRow(left, width, rows->head);
  List(PaneRow) *tail = drawPane(height - 1, left, width, rows->tail);
  head->tail = tail;
  return head;
}

List(PaneRow) *paneDraw(Pane *p, int *height, int *width) {
  RowList *rows = zipperRowsFrom(p->file->buffer, p->file->cursorY, p->top);
  return drawPane(*height, p->left, *width, rows);
}

PaneRow *makePaneRow(char *row, int width, unsigned int blanks) {
  PaneRow *r = malloc(sizeof(PaneRow));
  r->row = row;
  r->width = width;
  r->blanks = blanks;
  return r;
}

/*
Want to be able to say:
- draw this buffer on this pane
- draw this pane on this screen

Different panes should be able to:
- navigate to different places in the buffer
- have different sizes
- be positioned by their parent (probably a separate Window object?)

I like the idea of drawing on a pane being a function that's basically
draw :: Buffer -> Pane -> [Line], where the lines are what should be drawn.
I'm a little worried about doing a lot of work between the buffer and the
screen for no reason, and slowing the whole thing down.

Questions to ask:
- What's the ideal format/structure for the parent to receive in order to
  combine panes onto the screen?
  - Will be plopping it all into an append buffer, so just need to be able to
    memcpy it into place.
  - Readonly view (keeping wrapping, rendering on the buffer side) (?)
  -> So probably a linked list of pointers to characters in original lines
     to start at, and a number of characters to read. Pane should make sure
     this is a safe number of characters to read – parent will append blanks
     as needed.
  => Buffer -> Pane -> [(String, Width)]
- What's the ideal format/structure for a pane to get from a buffer?
  - Linked list of lines, starting at (bufferCursor - paneCursor)
  -> Buffers need a function: lines :: RelativeIndex -> LinkedList[Line]
 */
