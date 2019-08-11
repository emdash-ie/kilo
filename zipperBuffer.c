#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "editorRow.h"
#include "zipperBuffer.h"

int rowListId = 0;

RowList *newRowList(EditorRow *head, RowList *tail) {
  RowList *rowList = malloc(sizeof(RowList));
  rowList->head = head;
  rowList->tail = tail;
  rowList->number = rowListId++;
  return rowList;
}

/**
 * True if x is newer than y, false otherwise.
 */
bool rowListNewer(RowList *x, RowList *y) {
  if (y == NULL) {
    return true;
  } else if (x == NULL) {
    return false;
  } else {
    return x->number > y->number;
  }
}

void updateRowList(RowList *toUpdate, EditorRow *newHead, RowList *newTail) {
  if (toUpdate == NULL) return;
  toUpdate->head = newHead;
  toUpdate->tail = newTail;
  toUpdate->number = rowListId++;
}

/**
 * Reverses a RowList in-place. Returns the new head.
 */
RowList *rowListReverse(RowList *rows) {
  RowList *last = NULL;
  RowList *next = NULL;
  while (rows != NULL) {
    next = rows->tail;
    rows->tail = last;
    last = rows;
    rows = next;
  }
  return last;
}

void zipperForwardRow(ZipperBuffer *buffer, RowList *keepBefore) {
  if (buffer->forwards == NULL) return;
  RowList *oldForwards = buffer->forwards;
  RowList *newForwards = buffer->forwards->tail;
  RowList *oldBackwards = buffer->backwards;
  RowList *newBackwards;
  if (rowListNewer(oldForwards, keepBefore)) {
    updateRowList(oldForwards, oldForwards->head, oldBackwards);
    newBackwards = oldForwards;
  } else {
    newBackwards = newRowList(oldForwards->head, oldBackwards);
  }
  buffer->forwards = newForwards;
  buffer->backwards = newBackwards;
}

void zipperForwardN(ZipperBuffer *buffer, RowList *keepBefore, int n) {
  for (;n > 0; n--) {
    zipperForwardRow(buffer, keepBefore);
  }
}

void zipperBackwardRow(ZipperBuffer *buffer, RowList *keepBefore) {
  if (buffer->backwards == NULL) return;
  RowList *oldBackwards = buffer->backwards;
  RowList *newBackwards = buffer->backwards->tail;
  RowList *oldForwards = buffer->forwards;
  RowList *newForwards;
  if (rowListNewer(oldBackwards, keepBefore)) {
    updateRowList(oldBackwards, oldBackwards->head, oldForwards);
    newForwards = oldBackwards;
  } else {
    newForwards = newRowList(oldBackwards->head, oldForwards);
  }
  buffer->forwards = newForwards;
  buffer->backwards = newBackwards;
}

void zipperBackwardN(ZipperBuffer *buffer, RowList *keepBefore, int n) {
  for (;n > 0; n--) {
    zipperBackwardRow(buffer, keepBefore);
  }
}

void zipperInsertRow(ZipperBuffer *buffer, EditorRow *new) {
  RowList *oldForwards = buffer->forwards;
  RowList *newForwards = newRowList(new, oldForwards);
  buffer->forwards = newForwards;
}

void printRowList(RowList *list) {
  int i = 1;
  while (list != NULL) {
    printf("%d: %s\n", i, list->head->chars);
    list = list->tail;
    i++;
  }
}

void printZipperBuffer(ZipperBuffer *buffer) {
  printf("Backwards:\n");
  printRowList(buffer->backwards);
  printf("Forwards:\n");
  printRowList(buffer->forwards);
}

ZipperBuffer *exampleBuffer() {
  /* EditorRow *first = newRow("That's great, it starts with an earthquake.", 0); */
  /* EditorRow *second = newRow("Birds and snakes, an aeroplane.", 0); */
  /* EditorRow *third = newRow("Lenny Bruce is not afraid.", 0); */
  /* EditorRow *fourth = newRow("Eye of a hurricane, listen to yourself churn,", 0); */
  /* EditorRow *fifth = newRow("World serves its own needs, dummy serve your own needs", 0); */
  /* EditorRow *sixth = newRow("Feed it off an aux speak, grunt no strength", 0); */
  /* EditorRow *seventh = newRow("The ladder starts to clatter with fear of fight, down height", 0); */
  /* RowList *forwards = newRowList(first, */
  /*   newRowList(second, */
  /*   newRowList(third, */
  /*   newRowList(fourth, */
  /*   newRowList(fifth, */
  /*   newRowList(sixth, */
  /*   newRowList(seventh, NULL))))))); */
  ZipperBuffer *buffer = malloc(sizeof(ZipperBuffer));
  buffer->forwards = NULL;
  buffer->backwards = NULL;
  return buffer;
}

int testZipperBuffer() {
  ZipperBuffer *buffer = exampleBuffer();
  int i = 0;
  printf("Starting off:\n");
  printZipperBuffer(buffer);
  while (1) {
    printf("Iteration %d\n", ++i);
    while (buffer->forwards != NULL) {
      zipperForwardRow(buffer, NULL);
    }
    while (buffer->backwards != NULL) {
      zipperBackwardRow(buffer, NULL);
    }
  }
}
