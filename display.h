#pragma once

#include "linkedList.h"
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

DeclareList(DisplayRow);
DeclareList(Pane);
DeclareList(List(PaneRow));
DeclareList(List(List(PaneRow)));
DeclareList(int);
DeclareListFunctions2(Pane, List(PaneRow));
DeclareListFunctions4(Pane, int, int, List(PaneRow));
DeclareListFunctions4(DisplayRow, int, int, List(List(PaneRow)));
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
