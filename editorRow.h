#ifndef EDITOR_ROW
#define EDITOR_ROW

typedef struct EditorRow EditorRow;

struct EditorRow {
  int size;
  char *chars;
  int renderSize;
  char *renderChars;
};

int editorCursorToRender(EditorRow *row, int cursorX, int tabSize);

/**
 * Update the rendered characters for a row.
 */
void editorUpdateRow(EditorRow *row, int tabSize);

void editorFreeRow(EditorRow *row);

#endif
