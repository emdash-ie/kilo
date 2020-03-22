#include "editorRow.h"
#include "pane.h"
#include "util.h"
#include "zipperBuffer.h"

PaneContents *paneContentsCons(char *row, int width, PaneContents *tail) {
  PaneContents *contents = malloc(sizeof(PaneContents));
  contents->row = row;
  contents->width = width;
  contents->tail = tail;
  return contents;
}

PaneContents *drawRow(int left, int width, EditorRow *r) {
  int x = clip(left, 0, r->renderSize);
  int resultWidth = clip(r->renderSize - x, 0, width);
  return paneContentsCons(r->renderChars + x, resultWidth, NULL);
}

PaneContents *drawPane(int height, int left, int width, RowList *rows) {
  if (rows == NULL || height <= 0) {
    return NULL;
  }
  PaneContents *head = drawRow(left, width, rows->head);
  PaneContents *tail = drawPane(height - 1, left, width, rows->tail);
  head->tail = tail;
  return head;
}

PaneContents *draw(Pane *p, RowList *rows) {
  return drawPane(p->height, p->left, p->width, rows);
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
