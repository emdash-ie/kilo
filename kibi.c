/*** includes ***/

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _BSD_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

#include "display.h"
#include "editorRow.h"
#include "fileData.h"
#include "pane.h"
#include "undo.h"
#include "zipperBuffer.h"

DeclareListFunctionStruct(DisplayRow)

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define KIBI_VERSION "0.0.1"
#define tabSize 4

enum EditorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_DOWN,
  ARROW_UP,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY,
  DELETE_KEY
};

/*** data ***/

typedef struct EditorConfig {
  Display display;
  char statusMessage[80];
  time_t statusMessageTime;
  struct termios original_termios;
  void (*log)(char *format, ...);
  FILE *logFile;
} EditorConfig;

EditorConfig editor;

/*** prototypes ***/

void editorForwardLine();

void editorSetStatusMessage(const char *format, ...);

/*** logging ***/

void stderrLog(char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

void fileLog(char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(editor.logFile, format, args);
  va_end(args);
}

void noLog(char *format, ...) {
  return;
}

/*** terminal ***/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor.original_termios) == -1) {
    die("Failed to disable raw mode");
  }
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &editor.original_termios) == -1) {
    die("Failed to get terminal attributes while enabling raw mode");
  }
  atexit(disableRawMode);

  struct termios raw = editor.original_termios;
  raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("Failed to set terminal attributes while enabling raw mode");
  }
}

int editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("Error while reading input");
  }
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    if (seq[0] == '[' || seq[0] == 'O') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
          case '1': return HOME_KEY;
          case '3': return DELETE_KEY;
          case '4': return END_KEY;
          case '5': return PAGE_UP;
          case '6': return PAGE_DOWN;
          case '7': return HOME_KEY;
          case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
        case 'F': return END_KEY;
        case 'H': return HOME_KEY;
        }
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

int getCursorPosition(int *rows, int *columns) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, columns) != 2) return -1;

  return 0;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** undo ***/

void editorUndoSteps(UndoStack *undo) {
  int n = 0;
  while (undo != NULL) {
    n++;
    undo = undo->tail;
  }
  editorSetStatusMessage("%d undo steps.", n);
}

/*** row operations ***/

void editorInsertRow(
  char *s,
  size_t length,
  bool pushUndo,
  ZipperBuffer *buffer,
  int *numberOfRows,
  int *unsavedChanges,
  UndoStack **undo,
  int cursorX,
  int cursorY
) {
  if (pushUndo) {
    editorPushUndo(buffer, undo, cursorX, cursorY);
  }
  zipperInsertRow(buffer, newRow(s, length, tabSize));
  *numberOfRows = *numberOfRows + 1;
  *unsavedChanges = *unsavedChanges + 1;
}

void editorInsertRowAfter(
  char *s,
  size_t length,
  bool pushUndo,
  ZipperBuffer *buffer,
  int *numberOfRows,
  int *unsavedChanges,
  UndoStack **undo,
  int cursorX,
  int *cursorY
) {
  if (pushUndo) {
    editorPushUndo(buffer, undo, cursorX, *cursorY);
  }
  editorForwardLine();
  editorInsertRow(s, length, false, buffer, numberOfRows, unsavedChanges, undo, cursorX, *cursorY);
  if (*cursorY < activeHeight(&editor.display) - 1) {
    *cursorY = *cursorY + 1;
  }
}

void editorAppendRow(
  char *s,
  size_t length,
  bool pushUndo,
  ZipperBuffer *buffer,
  int *numberOfRows,
  int *unsavedChanges,
  UndoStack **undo,
  int cursorX,
  int cursorY
) {
  int i = 0;
  while (buffer->forwards != NULL) {
    zipperForwardRow(buffer);
    i++;
  }
  editorInsertRow(s, length, pushUndo, buffer, numberOfRows, unsavedChanges, undo, cursorX, cursorY);
  while (i > 0) {
    zipperBackwardRow(buffer);
    i--;
  }
}

void editorDeleteBetween(int startRow, int startColumn, int endRow, int endColumn) {

}

void editorDeleteCurrentRow(
  ZipperBuffer *buffer,
  UndoStack **undo,
  int *numberOfRows,
  int *unsavedChanges,
  int cursorX,
  int cursorY
) {
  if (buffer->forwards == NULL) return;
  editorPushUndo(buffer, undo, cursorX, cursorY);
  buffer->forwards = buffer->forwards->tail;
  numberOfRows--;
  unsavedChanges++;
}

void editorDeleteRow(
  ZipperBuffer *buffer,
  UndoStack **undo,
  int at,
  int *numberOfRows,
  int *unsavedChanges,
  int cursorX,
  int cursorY
) {
  if (at < 0 || at >= *numberOfRows) {
    return;
  }
  int moves = 0;
  while (buffer->backwards != NULL) {
    zipperBackwardRow(buffer);
    moves--;
  }
  moves += at;
  while (at > 0) {
    zipperForwardRow(buffer);
    at--;
  }
  editorDeleteCurrentRow(buffer, undo, numberOfRows, unsavedChanges, cursorX, cursorY);
  while (moves < -1) {
    zipperForwardRow(buffer);
    moves++;
  }
  while (moves > 0) {
    zipperBackwardRow(buffer);
  }
}

EditorRow *editorRowInsertChar(EditorRow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  char *newChars = malloc(row->size + 2);
  memcpy(newChars, row->chars, at);
  memcpy(&newChars[at + 1], &row->chars[at], row->size - at);
  newChars[at] = c;
  newChars[row->size + 1] = '\0';
  return newRow(newChars, row->size + 1, tabSize);
}

EditorRow *editorRowAppendString(EditorRow *row, char *s, size_t length) {
  char *newChars = malloc(row->size + length + 1);
  memcpy(newChars, row->chars, row->size);
  memcpy(&newChars[row->size], s, length);
  newChars[row->size + length] = '\0';
  return newRow(newChars, row->size + length, tabSize);
}

EditorRow *editorRowDeleteChar(EditorRow *row, int at) {
  if (at < 0 || at >= row->size) return row;
  char *newChars = malloc(row->size);
  memcpy(newChars, row->chars, at);
  memcpy(&newChars[at], &row->chars[at + 1], row->size - at);
  newChars[row->size - 1] = '\0';
  return newRow(newChars, row->size - 1, tabSize);
}

/**
 * Create a new row with the first n characters of row.
 */
EditorRow *editorRowTake(EditorRow *row, unsigned int n) {
  char *newChars = malloc(n + 1);
  memcpy(newChars, row->chars, n);
  newChars[n] = '\0';
  return newRow(newChars, n, tabSize);
}

/**
 * Create a new row with all characters of row after the first n.
 */
EditorRow *editorRowDrop(EditorRow *row, unsigned int n) {
  char *newChars = malloc(row->size - n + 1);
  memcpy(newChars, &row->chars[n], row->size - n);
  newChars[row->size - n] = '\0';
  return newRow(newChars, row->size - n, tabSize);
}

/**
 * Split a row at an index, return a RowList of the two new rows.
 */
RowList *editorRowSplit(EditorRow *row, unsigned int at) {
  EditorRow *first = editorRowTake(row, at);
  EditorRow *second = editorRowDrop(row, at);
  return rowListCons(first, rowListCons(second, NULL));
}

EditorRow *editorCurrentRow(ZipperBuffer *buffer) {
  return buffer->forwards ? buffer->forwards->head : NULL;
}

EditorRow *editorPreviousRow(ZipperBuffer *buffer) {
  return buffer->backwards ? buffer->backwards->head : NULL;
}

/*** editor operations ***/

void editorForwardLine(ZipperBuffer *buffer, int *cursorY) {
  if (editorCurrentRow(buffer) != NULL) {
    *cursorY += 1;
    zipperForwardRow(buffer);
  }
}

void editorBackwardLine(ZipperBuffer *buffer, int *cursorY) {
  if (editorPreviousRow(buffer) != NULL) {
    *cursorY -= 1;
    zipperBackwardRow(buffer);
  }
}

/**
 * Replace the current row with a new one.
 */
void editorReplaceRow(
  ZipperBuffer *buffer,
  UndoStack **undo,
  int cursorX,
  int cursorY,
  int *unsavedChanges,
  EditorRow *row
) {
  if (row == NULL) return;
  editorPushUndo(buffer, undo, cursorX, cursorY);
  RowList *old = buffer->forwards;
  if (old == NULL) {
    buffer->forwards = rowListCons(row, NULL);
  } else {
    buffer->forwards = rowListCons(row, old->tail);
  }
  *unsavedChanges = *unsavedChanges + 1;
}

void editorInsertChar(
  int c,
  ZipperBuffer *buffer,
  UndoStack **undo,
  int *numberOfRows,
  int *unsavedChanges,
  int *cursorX,
  int cursorY
) {
  EditorRow *row = editorCurrentRow(buffer);
  if (row == NULL) {
    editorInsertRow("", 0, true, buffer, numberOfRows, unsavedChanges, undo, *cursorX, cursorY);
    row = editorCurrentRow(buffer);
  }
  EditorRow *new = editorRowInsertChar(row, *cursorX, c);
  editorReplaceRow(buffer, undo, *cursorX, cursorY, unsavedChanges, new);
  *cursorX = *cursorX + 1;
}

void editorInsertRows(ZipperBuffer *buffer, UndoStack **undo, int cursorX, int cursorY, RowList *new, int *unsavedChanges) {
  if (new == NULL) return;
  editorPushUndo(buffer, undo, cursorX, cursorY);
  RowList *end = new;
  int added = 1;
  while (end->tail != NULL) {
    end = end->tail;
    added++;
  }
  end->tail = buffer->forwards;
  buffer->forwards = new;
  *unsavedChanges += added;
}

void editorInsertNewline(
  ZipperBuffer *buffer,
  UndoStack **undo,
  int *cursorX,
  int *cursorY,
  int *numberOfRows,
  int *unsavedChanges
) {
  EditorRow *row = editorCurrentRow(buffer);
  if (*cursorX == 0 || row == NULL) {
    editorInsertRowAfter("", 0, true, buffer, numberOfRows, unsavedChanges, undo, *cursorX, cursorY);
  } else {
    RowList *new = editorRowSplit(row, *cursorX);
    editorDeleteCurrentRow(buffer, undo, numberOfRows, unsavedChanges, *cursorX, *cursorY);
    editorInsertRows(buffer, undo, *cursorX, *cursorY, new, unsavedChanges);

    editorForwardLine(buffer, cursorY);
    *cursorX = 0;
  }
}

void editorDeleteChar(
  ZipperBuffer *buffer,
  UndoStack **undo,
  int *cursorX,
  int *cursorY,
  int *unsavedChanges,
  int *numberOfRows
) {
  EditorRow *current = editorCurrentRow(buffer);
  if (current == NULL) return;
  EditorRow *previous = editorPreviousRow(buffer);
  if (previous == NULL && *cursorX == 0) return;
  if (*cursorX > 0) {
    EditorRow *new = editorRowDeleteChar(current, *cursorX - 1);
    editorReplaceRow(buffer, undo, *cursorX, *cursorY, unsavedChanges, new);
    *cursorX -= 1;
  } else {
    *cursorX = previous->size;
    EditorRow *new = editorRowAppendString(previous,
                                           current->chars,
                                           current->size);
    editorDeleteCurrentRow(buffer, undo, numberOfRows, unsavedChanges, *cursorX, *cursorY);
    editorBackwardLine(buffer, cursorY);
    editorReplaceRow(buffer, undo, *cursorX, *cursorY, unsavedChanges, new);
  }
}

void editorJumpToEnd(
  ZipperBuffer *buffer,
  int *cursorY
) {
  while (editorCurrentRow(buffer) != NULL) {
    editorForwardLine(buffer, cursorY);
  }
}

void editorJumpToStart(
  ZipperBuffer *buffer,
  int *cursorY
) {
  while (editorPreviousRow(buffer) != NULL) {
    editorBackwardLine(buffer, cursorY);
  }
}

/*** file i/o ***/

char *editorRowsToString(ZipperBuffer *editorBuffer, int *bufferLength) {
  int rowsToEnd = 0;
  while (editorBuffer->forwards != NULL) {
    zipperForwardRow(editorBuffer);
    rowsToEnd++;
  }
  int totalLength = 0;
  while (editorBuffer->backwards != NULL) {
    totalLength += editorBuffer->backwards->head->size + 1;
    zipperBackwardRow(editorBuffer);
  }
  *bufferLength = totalLength;

  char *buffer = malloc(totalLength);
  char *p = buffer;
  while (editorBuffer->forwards != NULL) {
    memcpy(p, editorBuffer->forwards->head->chars,
           editorBuffer->forwards->head->size);
    p += editorBuffer->forwards->head->size;
    *p = '\n';
    p++;
    zipperForwardRow(editorBuffer);
  }
  while (rowsToEnd > 0) {
    zipperBackwardRow(editorBuffer);
    rowsToEnd--;
  }
  return buffer;
}

void editorOpen(
  char *filename,
  char **editorFilename,
  ZipperBuffer *buffer,
  UndoStack **undo,
  int *unsavedChanges,
  int *numberOfRows,
  int cursorX,
  int cursorY
) {
  free(*editorFilename);
  *editorFilename = strdup(filename);
  FILE *fp = fopen(filename, "r");
  if (!fp) die("Couldn't open file");
  char *line = NULL;
  size_t linecap = 0;
  ssize_t lineLength;
  while ((lineLength = getline(&line, &linecap, fp)) != -1) {
    while (lineLength > 0 &&
           (line[lineLength - 1] == '\n' || line[lineLength - 1] == '\r')) {
      lineLength--;
    }
    char *rowChars = malloc(lineLength + 1);
    memcpy(rowChars, line, lineLength);
    rowChars[lineLength + 1] = '\0';
    editorInsertRow(rowChars, lineLength, false, buffer, numberOfRows, unsavedChanges, undo, cursorX, cursorY);
  }
  buffer->forwards = rowListReverse(buffer->forwards);
  free(line);
  fclose(fp);
  *unsavedChanges = 0;
}

void editorSave(ZipperBuffer *editorBuffer, char *filename, int *unsavedChanges) {
  if (filename == NULL) return;
  int length;
  char *buffer = editorRowsToString(editorBuffer, &length);
  int fileDescriptor = open(filename, O_RDWR | O_CREAT, 0644);
  if (fileDescriptor != -1) {
    if (ftruncate(fileDescriptor, length) != -1) {
      if (write(fileDescriptor, buffer, length) == length) {
        close(fileDescriptor);
        free(buffer);
        editorSetStatusMessage("%d bytes written to disk", length);
        *unsavedChanges = 0;
        return;
      }
    }
    close(fileDescriptor);
  }
  free(buffer);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/*** append buffer ***/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) {
    return;
  }
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** output ***/

/**
 * Split the current pane in two, with the new (non-focused) split below the
 * current one.
 */
void splitBelow(Display *display) {
  int upperHeight = display->height / 2;
  int lowerHeight = display->height - upperHeight;
  int x = display->panes->active->active->cursorX;
  int y = display->panes->active->active->cursorY;
  int top = display->panes->active->active->top;
  int left = display->panes->active->active->left;
  FileData *file = display->panes->active->active->file;
  Pane *newPane = makePane(x, y, top, left, file);
  DisplayRow *newRow = makeDisplayRow(NULL, newPane, NULL);
  display->panes->down = ListF(DisplayRow).cons(newRow, display->panes->down);
}

void editorScroll(Pane *pane) {
  pane->cursorX = 0;
  EditorRow *current = editorCurrentRow(pane->file->buffer);
  if (current != NULL) {
    pane->cursorX = editorCursorToRender(current, pane->file->cursorX, tabSize);
  }
  if (pane->cursorX < pane->left) {
    pane->left = pane->cursorX;
  }
  if (pane->cursorX >= pane->left + activeWidth(&editor.display)) {
    pane->left = pane->cursorX - activeWidth(&editor.display) + 1;
  }
  pane->cursorY = pane->file->cursorY - pane->top;
  if (pane->cursorY < 0) {
    pane->top += pane->cursorY;
  }
  if (pane->cursorY >= activeHeight(&editor.display)) {
    pane->top = pane->file->cursorY - activeHeight(&editor.display) + 1;
  }
  pane->cursorY = pane->file->cursorY - pane->top;
}

void editorDrawString(struct abuf *ab, char *s, int length) {
  abAppend(ab, s, length);
}

void editorDrawBlanks(struct abuf *ab, int n) {
  for (; n > 0; n--) {
    abAppend(ab, " ", 1);
  }
}

void editorDrawNewline(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  abAppend(ab, "\r\n", 2);
}

void editorDrawLine(struct abuf *ab, char *s, int length) {
  editorDrawString(ab, s, length);
  editorDrawNewline(ab);
}

void editorDrawEmpties(struct abuf *ab, int numberOfLines) {
  editorDrawLine(ab, "~", 1);
  if (numberOfLines > 1) {
    editorDrawEmpties(ab, numberOfLines - 1);
  }
}

void editorDrawStatusBar(struct abuf *ab, int top, int left, int height, int width, int cursorX, int cursorY, int fileCursorX, int fileCursorY) {
  char status[80], rightStatus[80];
  int length = snprintf(status, sizeof(status), "\"%.20s\" - %d lines %s",
                        activePane(&editor.display)->file->filename
                        ? activePane(&editor.display)->file->filename
                          : "[No name]",
                        activePane(&editor.display)->file->numberOfRows,
                        activePane(&editor.display)->file->unsavedChanges ? "(modified)" : "");
  int rightLength = snprintf(rightStatus, sizeof(rightStatus), "(%d,%d,%d,%d,%d,%d,%d,%d) %d/%d",
                             top, left, height, width, cursorX, cursorY, fileCursorX, fileCursorY,
                             activePane(&editor.display)->cursorY + 1, activePane(&editor.display)->file->numberOfRows);
  if (length > editor.display.width) length = editor.display.width;
  abAppend(ab, status, length);
  while (length < editor.display.width) {
    if (editor.display.width - length == rightLength) {
      abAppend(ab, rightStatus, rightLength);
      break;
    } else {
      abAppend(ab, " ", 1);
      length++;
    }
  }
  abAppend(ab, "\r\n", 2);
}

void editorDrawWelcome(struct abuf *ab) {
  editorDrawEmpties(ab, editor.display.height / 3 - 1);
  char welcome[80];
  int welcomeLength = snprintf(
                               welcome,
                               sizeof(welcome),
                               "Kibi editor - version %s",
                               KIBI_VERSION
                               );
  if (welcomeLength > editor.display.width) {
    welcomeLength = editor.display.width;
  }
  int padding = (editor.display.width - welcomeLength) / 2;
  if (padding) {
    abAppend(ab, "~", 1);
    padding--;
  }
  while (padding--) abAppend(ab, " ", 1);
  abAppend(ab, welcome, welcomeLength);
}

void editorDrawRows(struct abuf *ab) {
  if (activePane(&editor.display)->file->numberOfRows == 0) {
    editorDrawWelcome(ab);
  } else {
    List(List(List(PaneRow))) *paneRows =
      drawDisplayColumn(editor.display.panes, editor.display.height, editor.display.width);

    int linesDrawn = 0;
    List(List(List(PaneRow))) *rows = paneRows;
    // for each column
    while (rows != NULL && linesDrawn < editor.display.height) {
      List(List(PaneRow)) *panes = rows->head;
      // for each row in the column
      while (panes->head != NULL && linesDrawn < editor.display.height) {
        int charactersDrawn = 0;
        List(List(PaneRow)) *panes2 = panes;
        // for each pane in the row, print the current line
        while (panes2 != NULL) {
          List(PaneRow) *pane = panes2->head;
          int proposedWidth = pane->head->width + pane->head->blanks;
          int widthAvailable = editor.display.width - charactersDrawn;
          int rowWidth = pane->head->width > widthAvailable ? widthAvailable : pane->head->width;
          int totalWidth =
              proposedWidth > widthAvailable ? widthAvailable : proposedWidth;
          editorDrawString(ab, pane->head->row, rowWidth);
          if (rowWidth < totalWidth) {
            editorDrawBlanks(ab, totalWidth - rowWidth);
          }
          charactersDrawn += totalWidth;
          // move pane pointer to next row
          panes2->head = panes2->head->tail;
          // move to next pane
          panes2 = panes2->tail;
        }
        editorDrawNewline(ab);
        linesDrawn++;
      }
      rows = rows->tail;
    }
    if (linesDrawn < editor.display.height) {
      editorDrawEmpties(ab, editor.display.height - linesDrawn);
    }
  }
}


void editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int messageLength = strlen(editor.statusMessage);
  if (messageLength > editor.display.width) messageLength = editor.display.width;
  if (messageLength && time(NULL) - editor.statusMessageTime < 5) {
    abAppend(ab, editor.statusMessage, messageLength);
  }
}

void editorUpdateWindowSize() {
  if (getWindowSize(&editor.display.height, &editor.display.width) == -1)
    die("getWindowSize");
  editor.display.height -= 2;
}

void editorRefreshScreen() {
  editorUpdateWindowSize();
  editorScroll(activePane(&editor.display));
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab, activePane(&editor.display)->top, activePane(&editor.display)->left, editor.display.height, editor.display.width, activePane(&editor.display)->cursorX, activePane(&editor.display)->cursorY, activePane(&editor.display)->file->cursorX, activePane(&editor.display)->file->cursorY);
  editorDrawMessageBar(&ab);
  char buf[32];
  ScreenCursor c = activeCursor(&editor.display);
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", c.y, c.x);
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vsnprintf(editor.statusMessage, sizeof(editor.statusMessage), format, ap);
  va_end(ap);
  editor.statusMessageTime = time(NULL);
}

/*** input ***/

void editorSwitchPane() {
  // TODO
}

void editorMoveCursor(ZipperBuffer *buffer, int *cursorX, int *cursorY, int key) {
  EditorRow *row = editorCurrentRow(buffer);
  switch (key) {
  case ARROW_DOWN:
  case CTRL_KEY('n'):
    editorForwardLine(buffer, cursorY);
    break;
  case ARROW_UP:
  case CTRL_KEY('p'):
    editorBackwardLine(buffer, cursorY);
    break;
  case ARROW_RIGHT:
  case CTRL_KEY('f'):
    if (row && *cursorX < row->size) {
      *cursorX += 1;
    } else if (row && *cursorX == row->size) {
      editorForwardLine(buffer, cursorY);
      *cursorX = 0;
    }
    break;
  case ARROW_LEFT:
  case CTRL_KEY('b'):
    if (*cursorX > 0) {
      *cursorX -= 1;
    } else if (editorPreviousRow(buffer) != NULL) {
      editorBackwardLine(buffer, cursorY);
      *cursorX = buffer->forwards->head->size;
    }
    break;
  }

  row = editorCurrentRow(buffer);
  int rowLength = row ? row->size : 0;
  if (*cursorX > rowLength) {
    *cursorX = rowLength;
  }
}

void editorProcessKeypress() {
  static int quitTimes = 1;
  int c = editorReadKey();
  FileData *fileData = activePane(&editor.display)->file;

  switch (c) {
  case '\r':
    editorInsertNewline(
      fileData->buffer,
      &fileData->undo,
      &fileData->cursorX,
      &fileData->cursorY,
      &fileData->numberOfRows,
      &fileData->unsavedChanges
    );
    break;
  case CTRL_KEY('z'): {
    onFailure(editorUndo(fileData), editorSetStatusMessage);
    break;
  }
  case CTRL_KEY('y'):
    onFailure(editorRedo(fileData), editorSetStatusMessage);
    break;
  case CTRL_KEY('x'):
    editorUndoSteps(fileData->undo);
    break;
  case CTRL_KEY('q'):
    if (fileData->unsavedChanges && quitTimes > 0) {
      editorSetStatusMessage("There are unsaved changes. Press Ctrl-q again to quit.");
      quitTimes = 0;
      return;
    }
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  case CTRL_KEY('s'):
    editorSave(fileData->buffer, fileData->filename, &fileData->unsavedChanges);
    break;
  case HOME_KEY:
  case CTRL_KEY('a'):
    fileData->cursorX = 0;
    break;
  case END_KEY:
  case CTRL_KEY('e'):
    {
      EditorRow *current = editorCurrentRow(fileData->buffer);
      if (current != NULL) {
        fileData->cursorX = current->size;
      }
      break;
    }
  case BACKSPACE:
  case CTRL_KEY('h'):
    editorDeleteChar(
      fileData->buffer,
      &fileData->undo,
      &fileData->cursorX,
      &fileData->cursorY,
      &fileData->unsavedChanges,
      &fileData->numberOfRows
    );
    break;
  case DELETE_KEY:
    editorMoveCursor(fileData->buffer, &fileData->cursorX, &fileData->cursorY, ARROW_RIGHT);
    editorDeleteChar(
      fileData->buffer,
      &fileData->undo,
      &fileData->cursorX,
      &fileData->cursorY,
      &fileData->unsavedChanges,
      &fileData->numberOfRows
    );
    break;
  case PAGE_UP:
  case PAGE_DOWN:
  case CTRL_KEY('u'):
  case CTRL_KEY('d'):
    {
      if (c == PAGE_UP || c == CTRL_KEY('u')) {
        fileData->cursorY = activePane(&editor.display)->top;
      } else {
        fileData->cursorY = activePane(&editor.display)->top + activeHeight(&editor.display) - 1;
        if (fileData->cursorY > fileData->numberOfRows) {
          fileData->cursorY = fileData->numberOfRows;
        }
      }
      int times = activeHeight(&editor.display);
      while (times--) {
        editorMoveCursor(
          fileData->buffer,
          &fileData->cursorX,
          &fileData->cursorY,
          (c == PAGE_UP || c == CTRL_KEY('u')) ? ARROW_UP : ARROW_DOWN
        );
      }
    }
    break;
  case CTRL_KEY('g'):
    editorJumpToEnd(fileData->buffer, &fileData->cursorY);
    break;
  case ARROW_DOWN:
  case ARROW_UP:
  case ARROW_RIGHT:
  case ARROW_LEFT:
  case CTRL_KEY('n'):
  case CTRL_KEY('p'):
  case CTRL_KEY('f'):
  case CTRL_KEY('b'):
    editorMoveCursor(fileData->buffer, &fileData->cursorX, &fileData->cursorY, c);
    break;
  case '\x1b':
  case CTRL_KEY('l'):
    break;
  case CTRL_KEY('w'):
    editorSwitchPane();
    break;
  default:
    editorInsertChar(
      c,
      fileData->buffer,
      &fileData->undo,
      &fileData->numberOfRows,
      &fileData->unsavedChanges,
      &fileData->cursorX,
      fileData->cursorY
    );
  }
  quitTimes = 1;
}

/*** init ***/

void initEditor() {
  ZipperBuffer *emptyBuffer = malloc(sizeof(ZipperBuffer));
  emptyBuffer->forwards = NULL;
  emptyBuffer->backwards = NULL;
  emptyBuffer->newest = NULL;
  FileData *emptyFile = fileData(0, 0, 0, emptyBuffer, NULL, 0, NULL, NULL);
  Pane *pane = makePane(0, 0, 0, 0, emptyFile);
  DisplayRow *row = makeDisplayRow(NULL, pane, NULL);
  DisplayColumn *column = makeDisplayColumn(NULL, row, NULL);
  editor.display = (Display){column, 0, 0};

  editor.statusMessage[0] = '\0';
  editor.statusMessageTime = 0;
  editor.log = stderrLog;

  editorUpdateWindowSize();
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    if (argc >= 4) {
      editor.log = fileLog;
      editor.logFile = fopen(argv[3], "w");
    } else {
      editor.log = noLog;
    }
    editorOpen(
      argv[1],
      &activePane(&editor.display)->file->filename,
      activePane(&editor.display)->file->buffer,
      &activePane(&editor.display)->file->undo,
      &activePane(&editor.display)->file->unsavedChanges,
      &activePane(&editor.display)->file->numberOfRows,
      activePane(&editor.display)->file->cursorX,
      activePane(&editor.display)->file->cursorY
    );
    splitBelow(&editor.display);
  }

  editorSetStatusMessage("Ctrl-q to quit, Ctrl-s to save");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  if (argc >= 4) {
    fclose(editor.logFile);
  }
  return 0;
}
