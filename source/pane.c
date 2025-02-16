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

List(PaneRow) *drawPane(int height, int left, int width, PaneRow *status, RowList *rows) {
  if (height <= 0) {
    return NULL;
  } else if (height == 1) {
    return ListF(PaneRow).cons(status, NULL);
  } else if (rows == NULL) {
    List(PaneRow) *head = ListF(PaneRow).cons(
      makePaneRow("", width, width),
      drawPane(height - 1, left, width, status, NULL)
    );
    return head;
  } else {
    List(PaneRow) *head = drawRow(left, width, rows->head);
    List(PaneRow) *tail = drawPane(height - 1, left, width, status, rows->tail);
    head->tail = tail;
    return head;
  }
}

List(PaneRow) *paneDraw(Pane *p, int *height, int *width) {
  RowList *rows = zipperRowsFrom(p->file->buffer, p->file->cursorY, p->top);
  return drawPane(*height, p->left, *width, drawStatusBar(p, *width), rows);
}

PaneRow *drawStatusBar(Pane *p, int width) {
  int bufferWidth = width + 4 + 5 + 1;
  char *status = malloc(bufferWidth);
  snprintf(status, 5, "\x1b[7m");
  int leftLength = snprintf(
    status + 4,
    width + 1,
    "\"%.20s\" - %d lines %s",
    p->file->filename ? p->file->filename : "[No name]",
    p->file->numberOfRows,
    p->file->unsavedChanges ? "(modified)" : ""
  );
  char *rightStatus = malloc(bufferWidth);
  int rightLength = snprintf(
    rightStatus,
    width + 1,
    "%d/%d",
    p->cursorY + 1,
    p->file->numberOfRows
  );
  snprintf(rightStatus + width, 6, "\x1b[27m");
  int numberOfBlanks = width - (leftLength + rightLength);
  for (int i = leftLength + 4; i < leftLength + 4 + numberOfBlanks; i++) {
    status[i] = ' ';
  }
  snprintf(
    status + leftLength + 4 + numberOfBlanks,
    rightLength + 5 + 1,
    "%s",
    rightStatus
  );
  free(rightStatus);
  return makePaneRow(status, width, 0);
}

PaneRow *makePaneRow(char *row, int width, unsigned int blanks) {
  PaneRow *r = malloc(sizeof(PaneRow));
  r->row = row;
  r->width = width;
  r->blanks = blanks;
  return r;
}
