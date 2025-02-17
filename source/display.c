#include "display.h"

DisplayColumn *makeDisplayColumn(
                                 List(DisplayRow) *u, DisplayRow *a, List(DisplayRow) *d
                                 ) {
  DisplayColumn *column = malloc(sizeof(DisplayColumn));
  column->up = u;
  column->active = a;
  column->down = d;
  return column;
}

DisplayRow *makeDisplayRow(List(Pane) *l, Pane *a, List(Pane) *r) {
  DisplayRow *row = malloc(sizeof(DisplayRow));
  row->left = l;
  row->active = a;
  row->right = r;
  return row;
}

Pane *activePane(Display *d) {
  return d->panes->active->active;
}

int activeHeight(Display *d) {
  return d->height / displayColumnSize(d->panes);
}

int activeWidth(Display *d) {
  return d->width / displayRowSize(d->panes->active);
}

ScreenCursor activeCursor(Display *d) {
  int columnUnitSize = d->height / displayColumnSize(d->panes);
  int columnOffset = columnUnitSize * ListF(DisplayRow).length(d->panes->up);
  int rowUnitSize = d->width / displayRowSize(d->panes->active);
  int rowOffset = rowUnitSize * ListF(Pane).length(d->panes->active->left);
  return (ScreenCursor){
    rowOffset + activePane(d)->cursorX + 1,
    columnOffset + activePane(d)->cursorY + 1
  };
}

List(List(List(PaneRow))) *drawDisplayColumn(DisplayColumn *column, int height, int width) {
  int n = displayColumnSize(column);
  int *eachHeight = malloc(sizeof(int));
  *eachHeight = height / n;
  int *firstHeight = malloc(sizeof(int));
  *firstHeight = height - ((n - 1) * *eachHeight);
  List(int) *heights =
    ListF(int).cons(firstHeight, ListF(int).cons(eachHeight, NULL));
  heights->tail->tail = heights->tail;
  int *eachWidth = malloc(sizeof(int));
  *eachWidth = width;
  List(int) *widths = ListF(int).cons(eachWidth, NULL);
  widths->tail = widths;
  List(DisplayRow) *top = ListF(DisplayRow).reverse(column->up);
  List(DisplayRow) *rows = ListF(DisplayRow).concat(
    top,
    ListF(DisplayRow).cons(column->active, column->down)
  );
  List(List(List(PaneRow))) *result = ListF4(DisplayRow, int, int, List(List(PaneRow)))
    .zipWith(drawDisplayRow, rows, heights, widths);

  free(heights->tail);
  free(heights);
  free(widths);
  free(eachHeight);
  free(firstHeight);
  free(eachWidth);
  ListF(DisplayRow).free(top);
  ListF(DisplayRow).freeUntil(rows, column->down);

  return result;
}

List(List(PaneRow)) *drawDisplayRow(DisplayRow *row, int *height, int *width) {
  List(Pane) *above = ListF(Pane).reverse(row->left);
  List(Pane) *panes =
    ListF(Pane).concat(above, ListF(Pane).cons(row->active, row->right));
  int n = ListF(Pane).length(panes);
  int *eachWidth = malloc(sizeof(int));
  *eachWidth = *width / n;
  List(int) *widths = ListF(int).cons(eachWidth, NULL);
  widths->tail = widths;
  List(int) *heights = ListF(int).cons(height, NULL);
  heights->tail = heights;
  List(List(PaneRow)) *result =
    ListF4(Pane, int, int, List(PaneRow))
    .zipWith(paneDraw, panes, heights, widths);

  free(heights);
  free(widths);
  free(eachWidth);
  ListF(Pane).free(above);
  ListF(Pane).freeUntil(panes, row->right);

  return result;
}

int displayColumnSize(DisplayColumn *column) {
  if (column == NULL) {
    return 0;
  } else {
    return 1 + ListF(DisplayRow).length(column->up)
      + ListF(DisplayRow).length(column->down);
  }
}

int displayRowSize(DisplayRow *row) {
  if (row == NULL) {
    return 0;
  } else {
    return 1 + ListF(Pane).length(row->left) +
      ListF(Pane).length(row->right);
  }
}
