#pragma once
#include "zipperBuffer.h"

/**
 * Rectangular area onscreen, with a cursor.
 *
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

typedef struct PaneContents {
  char *row;
  int width;
  struct PaneContents *tail;
} PaneContents;

PaneContents *paneContentsCons(char *row, int width, PaneContents *tail);

PaneContents *draw(Pane *p, RowList *rows);
