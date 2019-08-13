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
#include "editorRow.h"
#include "zipperBuffer.h"
#include "undo.h"

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
  int cursorX, cursorY;
  int cursorRow;
  int cursorRenderX;
  int rowOffset;
  int columnOffset;
  int screenrows;
  int screencols;
  int numberOfRows;
  ZipperBuffer *buffer;
  char *filename;
  char statusMessage[80];
  time_t statusMessageTime;
  int unsavedChanges;
  struct termios original_termios;
  UndoStack *undo;
  UndoStack *redo;
} EditorConfig;

EditorConfig editor;

/*** prototypes ***/

void editorForwardLine();

void editorSetStatusMessage(const char *format, ...);

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

void editorPushUndo() {
  editor.undo = undoCons(editor.buffer->forwards,
                         editor.buffer->backwards,
                         editor.cursorX, editor.undo);
}

void editorPushRedo() {
  editor.redo = undoCons(editor.buffer->forwards,
                         editor.buffer->backwards,
                         editor.cursorX, editor.redo);
}

void editorUndo() {
  if (editor.undo == NULL) {
    editorSetStatusMessage("No further undo steps.");
    return;
  }
  editorPushRedo();
  UndoStack *oldUndo = editor.undo;
  editor.buffer->forwards = oldUndo->forwards;
  editor.buffer->backwards = oldUndo->backwards;
  editor.cursorX = oldUndo->cursorX;
  editor.undo = oldUndo->tail;
  free(oldUndo);
}

void editorRedo() {
  if (editor.redo == NULL) {
    editorSetStatusMessage("No further redo steps.");
    return;
  }
  editorPushUndo();
  UndoStack *oldRedo = editor.redo;
  editor.buffer->forwards = oldRedo->forwards;
  editor.buffer->backwards = oldRedo->backwards;
  editor.cursorX = oldRedo->cursorX;
  editor.redo = oldRedo->tail;
  free(oldRedo);
}

void editorUndoSteps() {
  int n = 0;
  UndoStack *temp = editor.undo;
  while (temp != NULL) {
    n++;
    temp = temp->tail;
  }
  editorSetStatusMessage("%d undo steps.", n);
}

/*** row operations ***/

void editorInsertRow(char *s, size_t length, bool pushUndo) {
  if (pushUndo) {
    editorPushUndo();
  }
  zipperInsertRow(editor.buffer, newRow(s, length, tabSize));
  editor.numberOfRows++;
  editor.unsavedChanges++;
}

void editorInsertRowAfter(char *s, size_t length, bool pushUndo) {
  editorForwardLine();
  editorInsertRow(s, length, pushUndo);
  if (editor.cursorY < editor.screenrows - 1) {
    editor.cursorY++;
  }
}

void editorAppendRow(char *s, size_t length, bool pushUndo) {
  int i = 0;
  while (editor.buffer->forwards != NULL) {
    zipperForwardRow(editor.buffer);
    i++;
  }
  editorInsertRow(s, length, pushUndo);
  while (i > 0) {
    zipperBackwardRow(editor.buffer);
    i--;
  }
}

void editorDeleteCurrentRow() {
  if (editor.buffer->forwards == NULL) return;
  editorPushUndo();
  editor.buffer->forwards = editor.buffer->forwards->tail;
  editor.numberOfRows--;
  editor.unsavedChanges++;
}

void editorDeleteRow(int at) {
  if (at < 0 || at >= editor.numberOfRows) {
    return;
  }
  int moves = 0;
  while (editor.buffer->backwards != NULL) {
    zipperBackwardRow(editor.buffer);
    moves--;
  }
  moves += at;
  while (at > 0) {
    zipperForwardRow(editor.buffer);
    at--;
  }
  editorDeleteCurrentRow();
  while (moves < -1) {
    zipperForwardRow(editor.buffer);
    moves++;
  }
  while (moves > 0) {
    zipperBackwardRow(editor.buffer);
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

EditorRow *editorCurrentRow() {
  return editor.buffer->forwards ? editor.buffer->forwards->head : NULL;
}

EditorRow *editorPreviousRow() {
  return editor.buffer->backwards ? editor.buffer->backwards->head : NULL;
}

/*** editor operations ***/

void editorForwardLine() {
  if (editorCurrentRow() != NULL) {
    zipperForwardRow(editor.buffer);
    editor.cursorRow++;
  }
}

void editorBackwardLine() {
  if (editorPreviousRow() != NULL) {
    zipperBackwardRow(editor.buffer);
    editor.cursorRow--;
  }
}

/**
 * Replace the current row with a new one.
 */
void editorReplaceRow(EditorRow *row) {
  if (row == NULL) return;
  editorPushUndo();
  RowList *old = editor.buffer->forwards;
  if (old == NULL) {
    editor.buffer->forwards = rowListCons(row, NULL);
  } else {
    editor.buffer->forwards = rowListCons(row, old->tail);
  }
  editor.unsavedChanges++;
}

void editorInsertChar(int c) {
  EditorRow *row = editorCurrentRow();
  if (row == NULL) {
    editorInsertRow("", 0, true);
    row = editorCurrentRow();
  }
  EditorRow *new = editorRowInsertChar(row, editor.cursorX, c);
  editorReplaceRow(new);
  editor.cursorX++;
}

void editorInsertRows(RowList *new) {
  if (new == NULL) return;
  editorPushUndo();
  RowList *end = new;
  int added = 1;
  while (end->tail != NULL) {
    end = end->tail;
    added++;
  }
  end->tail = editor.buffer->forwards;
  editor.buffer->forwards = new;
  editor.unsavedChanges += added;
}

void editorInsertNewline() {
  EditorRow *row = editorCurrentRow();
  if (editor.cursorX == 0 || row == NULL) {
    editorInsertRowAfter("", 0, true);
  } else {
    RowList *new = editorRowSplit(row, editor.cursorX);
    editorDeleteCurrentRow();
    editorInsertRows(new);

    editorForwardLine();
    editor.cursorX = 0;
    editor.cursorY++;
  }
}

void editorDeleteChar() {
  EditorRow *current = editorCurrentRow();
  if (current == NULL) return;
  EditorRow *previous = editorPreviousRow();
  if (previous == NULL && editor.cursorX == 0) return;
  if (editor.cursorX > 0) {
    EditorRow *new = editorRowDeleteChar(current, editor.cursorX - 1);
    editorReplaceRow(new);
    editor.cursorX--;
  } else {
    editor.cursorX = previous->size;
    EditorRow *new = editorRowAppendString(previous,
                                           current->chars,
                                           current->size);
    editorDeleteCurrentRow();
    editorBackwardLine();
    editorReplaceRow(new);
    editor.cursorY--;
  }
}

void editorJumpToEnd() {
  while (editorCurrentRow() != NULL) {
    editorForwardLine();
  }
  editor.cursorY = editor.screenrows - 1;
}

/*** file i/o ***/

char *editorRowsToString(int *bufferLength) {
  int rowsToEnd = 0;
  while (editor.buffer->forwards != NULL) {
    zipperForwardRow(editor.buffer);
    rowsToEnd++;
  }
  int totalLength = 0;
  while (editor.buffer->backwards != NULL) {
    totalLength += editor.buffer->backwards->head->size + 1;
    zipperBackwardRow(editor.buffer);
  }
  *bufferLength = totalLength;

  char *buffer = malloc(totalLength);
  char *p = buffer;
  while (editor.buffer->forwards != NULL) {
    memcpy(p, editor.buffer->forwards->head->chars,
           editor.buffer->forwards->head->size);
    p += editor.buffer->forwards->head->size;
    *p = '\n';
    p++;
  }
  while (rowsToEnd > 0) {
    zipperBackwardRow(editor.buffer);
    rowsToEnd--;
  }
  return buffer;
}

void editorOpen(char *filename) {
  free(editor.filename);
  editor.filename = strdup(filename);
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
    editorInsertRow(rowChars, lineLength, false);
  }
  editor.buffer->forwards = rowListReverse(editor.buffer->forwards);
  free(line);
  fclose(fp);
  editor.unsavedChanges = 0;
}

void editorSave() {
  if (editor.filename == NULL) return;
  int length;
  char *buffer = editorRowsToString(&length);
  int fileDescriptor = open(editor.filename, O_RDWR | O_CREAT, 0644);
  if (fileDescriptor != -1) {
    if (ftruncate(fileDescriptor, length) != -1) {
      if (write(fileDescriptor, buffer, length) == length) {
        close(fileDescriptor);
        free(buffer);
        editorSetStatusMessage("%d bytes written to disk", length);
        editor.unsavedChanges = 0;
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

void editorScroll() {
  editor.cursorRenderX = 0;
  EditorRow *current = editorCurrentRow();
  if (current != NULL) {
    editor.cursorRenderX = editorCursorToRender(current, editor.cursorX, tabSize);
  }
  if (editor.cursorRenderX < editor.columnOffset) {
    editor.columnOffset = editor.cursorRenderX;
  }
  if (editor.cursorRenderX >= editor.columnOffset + editor.screencols) {
    editor.columnOffset = editor.cursorRenderX - editor.screencols + 1;
  }
}

void editorDrawLine(struct abuf *ab, char *s, int length) {
  abAppend(ab, s, length);
  abAppend(ab, "\x1b[K", 3);
  abAppend(ab, "\r\n", 2);
}

void editorDrawRow(struct abuf *ab, EditorRow *row) {
  int length = row->renderSize - editor.columnOffset;
  if (length < 0) length = 0;
  if (length > editor.screencols) length = editor.screencols;
  editorDrawLine(ab, &row->renderChars[editor.columnOffset], length);
}

void editorDrawEmpties(struct abuf *ab, int numberOfLines) {
  editorDrawLine(ab, "~", 1);
  if (numberOfLines > 1) {
    editorDrawEmpties(ab, numberOfLines - 1);
  }
}

void editorDrawBackwards(struct abuf *ab, RowList* rows, int numberOfLines) {
  if (numberOfLines < 1) return;
  if (rows == NULL) {
    editorDrawEmpties(ab, numberOfLines);
    return;
  }
  if (numberOfLines > 1) {
    editorDrawBackwards(ab, rows->tail, numberOfLines - 1);
  }
  editorDrawRow(ab, rows->head);
}

void editorDrawForwards(struct abuf *ab, RowList* rows, int numberOfLines) {
  if (numberOfLines < 1) return;
  if (rows == NULL) {
    editorDrawEmpties(ab, numberOfLines);
    return;
  }
  editorDrawRow(ab, rows->head);
  if (numberOfLines > 1) {
    editorDrawForwards(ab, rows->tail, numberOfLines - 1);
  }
}

void editorDrawStatusBar(struct abuf *ab) {
  char status[80], rightStatus[80];
  int length = snprintf(status, sizeof(status), "\"%.20s\" - %d lines %s",
                        editor.filename ? editor.filename : "[No name]",
                        editor.numberOfRows,
                        editor.unsavedChanges ? "(modified)" : "");
  int rightLength = snprintf(rightStatus, sizeof(rightStatus), "%d/%d",
                             editor.cursorRow + 1, editor.numberOfRows);
  if (length > editor.screencols) length = editor.screencols;
  abAppend(ab, status, length);
  while (length < editor.screencols) {
    if (editor.screencols - length == rightLength) {
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
  editorDrawEmpties(ab, editor.screenrows / 3 - 1);
  char welcome[80];
  int welcomeLength = snprintf(
                               welcome,
                               sizeof(welcome),
                               "Kibi editor - version %s",
                               KIBI_VERSION
                               );
  if (welcomeLength > editor.screencols) {
    welcomeLength = editor.screencols;
  }
  int padding = (editor.screencols - welcomeLength) / 2;
  if (padding) {
    abAppend(ab, "~", 1);
    padding--;
  }
  while (padding--) abAppend(ab, " ", 1);
  abAppend(ab, welcome, welcomeLength);
}

void editorDrawRows(struct abuf *ab) {
  if (editor.numberOfRows == 0) {
    editorDrawWelcome(ab);
  } else {
    editorDrawBackwards(ab, editor.buffer->backwards, editor.cursorY);
    int toDraw = editor.screenrows - editor.cursorY;
    editorDrawForwards(ab, editor.buffer->forwards, toDraw);
  }
}


void editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int messageLength = strlen(editor.statusMessage);
  if (messageLength > editor.screencols) messageLength = editor.screencols;
  if (messageLength && time(NULL) - editor.statusMessageTime < 5) {
    abAppend(ab, editor.statusMessage, messageLength);
  }
}

void editorRefreshScreen() {
  editorScroll();
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
           editor.cursorY + 1,
           (editor.cursorRenderX - editor.columnOffset) + 1);
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

void editorMoveCursor(int key) {
  EditorRow *row = editorCurrentRow();
  switch (key) {
  case ARROW_DOWN:
  case CTRL_KEY('n'):
    if (editor.cursorY < editor.screenrows - 1) {
      editor.cursorY++;
    }
    editorForwardLine();
    break;
  case ARROW_UP:
  case CTRL_KEY('p'):
    if (editor.cursorY > 0) {
      editor.cursorY--;
    }
    editorBackwardLine();
    break;
  case ARROW_RIGHT:
  case CTRL_KEY('f'):
    if (row && editor.cursorX < row->size) {
      editor.cursorX++;
    } else if (row && editor.cursorX == row->size) {
      editor.cursorY++;
      editorForwardLine();
      editor.cursorX = 0;
    }
    break;
  case ARROW_LEFT:
  case CTRL_KEY('b'):
    if (editor.cursorX > 0) {
      editor.cursorX--;
    } else if (editorPreviousRow() != NULL) {
      if (editor.cursorY > 0) {
        editor.cursorY--;
      }
      editorBackwardLine();
      editor.cursorX = editor.buffer->forwards->head->size;
    }
    break;
  }

  row = editorCurrentRow();
  int rowLength = row ? row->size : 0;
  if (editor.cursorX > rowLength) {
    editor.cursorX = rowLength;
  }
}

void editorProcessKeypress() {
  static int quitTimes = 1;
  int c = editorReadKey();

  switch (c) {
  case '\r':
    editorInsertNewline();
    break;
  case CTRL_KEY('z'):
    editorUndo();
    break;
  case CTRL_KEY('y'):
    editorRedo();
    break;
  case CTRL_KEY('x'):
    editorUndoSteps();
    break;
  case CTRL_KEY('q'):
    if (editor.unsavedChanges && quitTimes > 0) {
      editorSetStatusMessage("There are unsaved changes. Press Ctrl-q again to quit.");
      quitTimes = 0;
      return;
    }
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  case CTRL_KEY('s'):
    editorSave();
    break;
  case HOME_KEY:
  case CTRL_KEY('a'):
    editor.cursorX = 0;
    break;
  case END_KEY:
  case CTRL_KEY('e'):
    {
      EditorRow *current = editorCurrentRow();
      if (current != NULL) {
        editor.cursorX = current->size;
      }
      break;
    }
  case BACKSPACE:
  case CTRL_KEY('h'):
    editorDeleteChar();
    break;
  case DELETE_KEY:
    editorMoveCursor(ARROW_RIGHT);
    editorDeleteChar();
    break;
  case PAGE_UP:
  case PAGE_DOWN:
  case CTRL_KEY('u'):
  case CTRL_KEY('d'):
    {
      if (c == PAGE_UP || c == CTRL_KEY('u')) {
        editor.cursorY = editor.rowOffset;
      } else {
        editor.cursorY = editor.rowOffset + editor.screenrows - 1;
        if (editor.cursorY > editor.numberOfRows) {
          editor.cursorY = editor.numberOfRows;
        }
      }
      int times = editor.screenrows;
      while (times--) {
        editorMoveCursor((c == PAGE_UP || c == CTRL_KEY('u')) ? ARROW_UP : ARROW_DOWN);
      }
    }
    break;
  case CTRL_KEY('g'):
    editorJumpToEnd();
    break;
  case ARROW_DOWN:
  case ARROW_UP:
  case ARROW_RIGHT:
  case ARROW_LEFT:
  case CTRL_KEY('n'):
  case CTRL_KEY('p'):
  case CTRL_KEY('f'):
  case CTRL_KEY('b'):
    editorMoveCursor(c);
    break;
  case '\x1b':
  case CTRL_KEY('l'):
    break;
  default:
    editorInsertChar(c);
  }
  quitTimes = 1;
}

/*** init ***/

void initEditor() {
  editor.cursorX = 0;
  editor.cursorY = 0;
  editor.cursorRenderX = 0;
  editor.rowOffset = 0;
  editor.columnOffset = 0;
  editor.numberOfRows = 0;
  editor.filename = NULL;
  editor.statusMessage[0] = '\0';
  editor.statusMessageTime = 0;
  editor.unsavedChanges = 0;
  editor.buffer = malloc(sizeof(*editor.buffer));
  editor.buffer->forwards = NULL;
  editor.buffer->backwards = NULL;
  editor.buffer->newest = NULL;
  editor.undo = NULL;
  editor.undo = NULL;
  editor.redo = NULL;

  if (getWindowSize(&editor.screenrows, &editor.screencols) == -1) die("getWindowSize");
  editor.screenrows -= 2;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage("Ctrl-q to quit, Ctrl-s to save");

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
