#pragma once
#include "zipperBuffer.h"

/**
 * Rectangular area onscreen, with a cursor.
 *
 * cursorX: x-position of the screen cursor, relative to the pane. Leftmost is 0.
 * cursorY: y-position of the screen cursor, relative to the pane. Topmost is 0.
 * top: top of pane starts this many lines from the top of the buffer
 * left: left of pane starts this many characters from the left of the
 *   buffer
 * width: width of the pane in characters
 * height: height of the pane in lines
 */
typedef struct Pane {
  int cursorX, cursorY;
  int top;
  int left;
  int width;
  int height;
} Pane;

Pane *makePane(int cursorX, int cursorY, int top, int left, int width, int height);

typedef struct PaneContents {
  char *row;
  int width;
  struct PaneContents *tail;
} PaneContents;

PaneContents *paneContentsCons(char *row, int width, PaneContents *tail);

PaneContents *paneDraw(Pane *p, RowList *rows);
