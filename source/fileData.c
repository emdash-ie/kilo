#include "fileData.h"
#include <stdlib.h>

FileData *fileData(int cursorX, int cursorY, int numberOfRows,
                   ZipperBuffer *buffer, char *filename, int unsavedChanges,
                   UndoStack *undo, UndoStack *redo) {
  FileData *fd = malloc(sizeof(FileData));
  fd->cursorX = cursorX;
  fd->cursorY = cursorY;
  fd->numberOfRows = numberOfRows;
  fd->buffer = buffer;
  fd->filename = filename;
  fd->unsavedChanges = unsavedChanges;
  fd->undo = undo;
  fd->redo = redo;
  return fd;
}

bool isSuccess(OperationResult *result) {
  if (result->type == Success) {
    return true;
  } else {
    return false;
  }
}

OperationResult *failure(char *message) {
  OperationResult *result = malloc(sizeof(OperationResult));
  result->type = Failure;
  result->errorMessage = message;
  return result;
}

OperationResult *success() {
  OperationResult *result = malloc(sizeof(OperationResult));
  result->type = Success;
  result->errorMessage = "";
  return result;
}

void onFailure(OperationResult *result, void (*f)(const char *, ...)) {
  if (!isSuccess(result)) {
    f(result->errorMessage);
    free(result);
  }
}

void editorPushRedo(
  ZipperBuffer *buffer,
  UndoStack **redo,
  int cursorX,
  int cursorY
) {
  *redo = undoCons(buffer->forwards, buffer->backwards, cursorX, cursorY, *redo);
}

void editorPushUndo(
  ZipperBuffer *buffer,
  UndoStack **undo,
  int cursorX,
  int cursorY
) {
  zipperUpdateNewest(buffer);
  *undo = undoCons(buffer->forwards, buffer->backwards, cursorX, cursorY, *undo);
}

OperationResult *editorUndo(FileData *file) {
  if (file->undo == NULL) {
    return failure("No further undo steps.");
  }
  editorPushRedo(file->buffer, &file->redo, file->cursorX, file->cursorY);
  UndoStack *oldUndo = file->undo;
  file->buffer->forwards = oldUndo->forwards;
  file->buffer->backwards = oldUndo->backwards;
  file->cursorX = oldUndo->cursorX;
  file->cursorY = oldUndo->cursorY;
  file->undo = oldUndo->tail;
  free(oldUndo);
  return success();
}

OperationResult *editorRedo(FileData *file) {
  if (file->redo == NULL) {
    return failure("No further redo steps.");
  }
  editorPushUndo(file->buffer, &file->undo, file->cursorX, file->cursorY);
  UndoStack *oldRedo = file->redo;
  file->buffer->forwards = oldRedo->forwards;
  file->buffer->backwards = oldRedo->backwards;
  file->cursorX = oldRedo->cursorX;
  file->cursorY = oldRedo->cursorY;
  file->redo = oldRedo->tail;
  free(oldRedo);
  return success();
}
