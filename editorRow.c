#include <stdlib.h>
#include <string.h>
#include "editorRow.h"

EditorRow *newRow(char *s, size_t length, int tabSize) {
  EditorRow *row = malloc(sizeof(*row));
  row->size = length;
  row->chars = malloc(length + 1);
  memcpy(row->chars, s, length);
  row->chars[length] = '\0';
  row->renderSize = 0;
  row->renderChars = NULL;
  editorUpdateRow(row, tabSize);
  return row;
}

int editorCursorToRender(EditorRow *row, int cursorX, int tabSize) {
  int renderX = 0;
  for (int j = 0; j < cursorX; j++) {
    if (row->chars[j] == '\t') {
      renderX += tabSize;
    } else {
      renderX += 1;
    }
  }
  return renderX;
}

/**
 * Update the rendered characters for a row.
 */
void editorUpdateRow(EditorRow *row, int tabSize) {
  int tabs = 0;
  for (int j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      tabs++;
    }
  }
  free(row->renderChars);
  row->renderChars = malloc(row->size + tabs * (tabSize - 1) + 1);
  int i = 0;
  for (int j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      for (int size = tabSize; size > 0; size--) {
        row->renderChars[i++] = ' ';
      }
    } else {
      row->renderChars[i++] = row->chars[j];
    }
  }
  row->renderChars[i] = '\0';
  row->renderSize = i;
}

void editorFreeRow(EditorRow *row) {
  free(row->renderChars);
  free(row->chars);
}
