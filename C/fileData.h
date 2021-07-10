#pragma once
#include <stdbool.h>

#include "undo.h"
#include "zipperBuffer.h"

/**
 * Data the editor needs for each open file. I should probably call this a
 * ~Buffer~.
 *
 * cursorX, cursorY: Position of the cursor within the file in characters.
 *   10, 42 is the 11th line from the top, 43rd column from the left
 * numberOfRows: Number of lines in the file.
 * buffer: The underlying file buffer.
 * filename: Full path of the file.
 * unsavedChanges: How many changes have been made since the last save.
 * undo, redo: The undo and redo data for the file.
 */
typedef struct FileData {
  int cursorX, cursorY;
  int numberOfRows;
  ZipperBuffer *buffer;
  char *filename;
  int unsavedChanges;
  UndoStack *undo;
  UndoStack *redo;
} FileData;

FileData *fileData(int cursorX, int cursorY, int numberOfRows,
                   ZipperBuffer *buffer, char *filename, int unsavedChanges,
                   UndoStack *undo, UndoStack *redo);

enum OperationResultType { Success, Failure };

/**
 * Empty on success, error message on failure.
 */
typedef struct OperationResult {
  enum OperationResultType type;
  char *errorMessage;
} OperationResult;

bool isSuccess(OperationResult *result);

OperationResult *failure(char *message);

OperationResult success;

void onFailure(OperationResult *result, void (*f)(const char *, ...));

void editorPushRedo(
  ZipperBuffer *buffer,
  UndoStack **redo,
  int cursorX,
  int cursorY
);

void editorPushUndo(
  ZipperBuffer *buffer,
  UndoStack **undo,
  int cursorX,
  int cursorY
);

OperationResult *editorUndo(FileData *file);

OperationResult *editorRedo(FileData *file);
