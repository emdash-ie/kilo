#pragma once

#include "lists/DisplayRow-int-int-ListListPaneRow.h"
#include "lists/DisplayRow.h"
#include "lists/ListListPaneRow.h"
#include "lists/ListPaneRow.h"
#include "lists/Pane-ListPaneRow.h"
#include "lists/Pane-int-int-ListPaneRow.h"
#include "lists/Pane.h"
#include "lists/int.h"
#include "pane.h"

typedef struct DisplayRow DisplayRow;
typedef struct DisplayColumn DisplayColumn;
typedef struct Display Display;

struct Display {
  DisplayColumn *panes;
  int height;
  int width;
};

typedef struct ScreenCursor {
  int x;
  int y;
} ScreenCursor;

struct DisplayColumn {
  List(DisplayRow) *up;
  DisplayRow *active;
  List(DisplayRow) *down;
};

struct DisplayRow {
  List(Pane) *left;
  Pane *active;
  List(Pane) *right;
};

int paneListSize(List(Pane) * ps);
int columnListSize(List(DisplayRow) * ps);
int displayColumnSize(DisplayColumn *row);
int displayRowSize(DisplayRow *row);
List(List(PaneRow)) * drawDisplayRow(DisplayRow *row, int *height, int *width);

DisplayRow *makeDisplayRow(List(Pane) *l, Pane *a, List(Pane) *r);
DisplayColumn *makeDisplayColumn(
  List(DisplayRow) *u, DisplayRow *a, List(DisplayRow) *d
);
Pane *activePane(Display *d);
int activeHeight(Display *d);
int activeWidth(Display *d);
ScreenCursor activeCursor(Display *d);

List(List(List(PaneRow))) *drawDisplayColumn(DisplayColumn *column, int height, int width);
