#pragma once
#include "fileData.h"
#include "lists/PaneRow.h"
#include "zipperBuffer.h"

/**
 * Rectangular area onscreen, with a cursor. Note that the cursor counts screen
 * spaces, and so must e.g. convert tabs to spaces.
 *
 * cursorX: x-position of the screen cursor, relative to the pane. Leftmost is 0.
 * cursorY: y-position of the screen cursor, relative to the pane. Topmost is 0.
 * top: top of pane starts this many lines from the top of the buffer
 * left: left of pane starts this many characters from the left of the
 *   buffer
 */
typedef struct Pane {
  int cursorX;
  int cursorY;
  int top;
  int left;
  FileData *file;
} Pane;

Pane *makePane(int cursorX, int cursorY, int top, int left, FileData *file);

typedef struct PaneRow {
  char *row;
  int width;
  /** The number of blanks needed to the right of the line. */
  int blanks;
} PaneRow;

PaneRow *makePaneRow(char *row, int width, unsigned int blanks);

PaneRow *drawStatusBar(Pane *p, int width);

List(PaneRow) *paneDraw(Pane *p, int *height, int *width);

List(PaneRow) *drawRow(int left, int width, EditorRow *r);
