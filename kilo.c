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

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define KIBI_VERSION "0.0.1"
#define tabSize 4

enum EditorKey {
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

typedef struct EditorRow EditorRow;

struct EditorRow {
  int size;
  char *chars;
  int renderSize;
  char *renderChars;
};

typedef struct EditorConfig {
  int cursorX, cursorY;
  int cursorRenderX;
  int rowOffset;
  int columnOffset;
  int screenrows;
  int screencols;
  int numberOfRows;
  EditorRow *rows;
  struct termios original_termios;
} EditorConfig;

EditorConfig editor;

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

/*** row operations ***/

int editorCursorToRender(EditorRow *row, int cursorX) {
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

void editorUpdateRow(EditorRow *row) {
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

void editorAppendRow(char *s, size_t length) {
  editor.rows = realloc(editor.rows, sizeof(EditorRow) * (editor.numberOfRows + 1));
  int new = editor.numberOfRows;
  editor.rows[new].size = length;
  editor.rows[new].chars = malloc(length + 1);
  memcpy(editor.rows[new].chars, s, length);
  editor.rows[new].chars[length] = '\0';
  editor.rows[new].renderSize = 0;
  editor.rows[new].renderChars = NULL;
  editorUpdateRow(&editor.rows[new]);
  editor.numberOfRows++;
}

/*** file i/o ***/

void editorOpen(char *filename) {
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
    editorAppendRow(line, lineLength);
  }
  free(line);
  fclose(fp);
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
  if (editor.cursorY < editor.numberOfRows) {
    editor.cursorRenderX = editorCursorToRender(&editor.rows[editor.cursorY], editor.cursorX);
  }

  if (editor.cursorY < editor.rowOffset) {
    editor.rowOffset = editor.cursorY;
  }
  if (editor.cursorY >= editor.rowOffset + editor.screenrows) {
    editor.rowOffset = editor.cursorY - editor.screenrows + 1;
  }
  if (editor.cursorRenderX < editor.columnOffset) {
    editor.columnOffset = editor.cursorRenderX;
  }
  if (editor.cursorRenderX >= editor.columnOffset + editor.screencols) {
    editor.columnOffset = editor.cursorRenderX - editor.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) {
  for (int y = 0; y < editor.screenrows; y++) {
    int fileRow = y + editor.rowOffset;
    if (fileRow >= editor.numberOfRows) {
      if (editor.numberOfRows == 0 && y == editor.screenrows / 3) {
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
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int length = editor.rows[fileRow].renderSize - editor.columnOffset;
      if (length < 0) {
        length = 0;
      }
      if (length > editor.screencols) {
        length = editor.screencols;
      }
      abAppend(ab, &editor.rows[fileRow].renderChars[editor.columnOffset], length);
    }
    abAppend(ab, "\x1b[K", 3);
    if (y < editor.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void editorRefreshScreen() {
  editorScroll();
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
           (editor.cursorY - editor.rowOffset) + 1,
           (editor.cursorRenderX - editor.columnOffset) + 1);
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** input ***/

void editorMoveCursor(int key) {
  EditorRow *row = (editor.cursorY >= editor.numberOfRows) ? NULL : &editor.rows[editor.cursorY];
  switch (key) {
  case ARROW_DOWN:
    if (editor.cursorY < editor.numberOfRows) {
      editor.cursorY++;
    }
    break;
  case ARROW_UP:
    if (editor.cursorY > 0) {
      editor.cursorY--;
    }
    break;
  case ARROW_RIGHT:
    if (row && editor.cursorX < row->size) {
    editor.cursorX++;
    } else if (row && editor.cursorX == row->size) {
      editor.cursorY++;
      editor.cursorX = 0;
    }
    break;
  case ARROW_LEFT:
    if (editor.cursorX > 0) {
      editor.cursorX--;
    } else if (editor.cursorY > 0) {
      editor.cursorY--;
      editor.cursorX = editor.rows[editor.cursorY].size;
    }
    break;
  }

  row = (editor.cursorY >= editor.numberOfRows) ? NULL : &editor.rows[editor.cursorY];
  int rowLength = row ? row->size : 0;
  if (editor.cursorX > rowLength) {
    editor.cursorX = rowLength;
  }
}

void editorProcessKeypress() {
  int c = editorReadKey();

  switch (c) {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  case HOME_KEY:
    editor.cursorX = 0;
    break;
  case END_KEY:
    editor.cursorX = editor.screencols - 1;
    break;
  case PAGE_UP:
  case PAGE_DOWN:
    {
      if (c == PAGE_UP) {
        editor.cursorY = editor.rowOffset;
      } else {
        editor.cursorY = editor.rowOffset + editor.screenrows - 1;
        if (editor.cursorY > editor.numberOfRows) {
          editor.cursorY = editor.numberOfRows;
        }
      }
      int times = editor.screenrows;
      while (times--) {
        editorMoveCursor(c = PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
    }
    break;
  case ARROW_DOWN:
  case ARROW_UP:
  case ARROW_RIGHT:
  case ARROW_LEFT:
    editorMoveCursor(c);
    break;
  }
}

/*** init ***/

void initEditor() {
  editor.cursorX = 0;
  editor.cursorY = 0;
  editor.cursorRenderX = 0;
  editor.rowOffset = 0;
  editor.columnOffset = 0;
  editor.numberOfRows = 0;
  editor.rows = NULL;
  if (getWindowSize(&editor.screenrows, &editor.screencols) == -1) die("getWindowSize");
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
