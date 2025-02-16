#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "editorRow.h"
#include "zipperBuffer.h"

RowList *rowListDrop(RowList *list, int n) {
  if (n <= 0) {
    return list;
  } else {
    return rowListDrop(list->tail, n - 1);
  }
}

int rowListId = 0;

RowList *rowListCons(EditorRow *head, RowList *tail) {
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

void zipperForwardRow(ZipperBuffer *buffer) {
  if (buffer->forwards == NULL) return;
  RowList *oldForwards = buffer->forwards;
  RowList *newForwards = buffer->forwards->tail;
  RowList *oldBackwards = buffer->backwards;
  RowList *newBackwards = rowListCons(oldForwards->head, oldBackwards);
  if (rowListNewer(oldForwards, buffer->newest)) {
    free(oldForwards);
  }
  buffer->forwards = newForwards;
  buffer->backwards = newBackwards;
}

void zipperForwardN(ZipperBuffer *buffer, int n) {
  for (;n > 0; n--) {
    zipperForwardRow(buffer);
  }
}

void zipperBackwardRow(ZipperBuffer *buffer) {
  if (buffer->backwards == NULL) return;
  RowList *oldBackwards = buffer->backwards;
  RowList *newBackwards = buffer->backwards->tail;
  RowList *oldForwards = buffer->forwards;
  RowList *newForwards = rowListCons(oldBackwards->head, oldForwards);
  if (rowListNewer(oldBackwards, buffer->newest)) {
    free(oldBackwards);
  }
  buffer->forwards = newForwards;
  buffer->backwards = newBackwards;
}

void zipperBackwardN(ZipperBuffer *buffer, int n) {
  for (;n > 0; n--) {
    zipperBackwardRow(buffer);
  }
}

void zipperInsertRow(ZipperBuffer *buffer, EditorRow *r) {
  RowList *oldForwards = buffer->forwards;
  RowList *newForwards = rowListCons(r, oldForwards);
  buffer->forwards = newForwards;
  buffer->newest = newForwards;
}

RowList *zipperRowsFromRelative(ZipperBuffer *buffer, int n) {
  if (n >= 0) {
    return rowListDrop(buffer->forwards, n);
  } else {
    zipperBackwardN(buffer, -n);
    RowList *rows = buffer->forwards;
    zipperUpdateNewest(buffer);
    zipperForwardN(buffer, -n);
    return rows;
  }
}

RowList *zipperRowsFrom(ZipperBuffer *buffer, int cursorY, int n) {
  return zipperRowsFromRelative(buffer, n - cursorY);
}

void zipperUpdateNewest(ZipperBuffer *buffer) {
  RowList *thisNewest = rowListNewer(buffer->forwards, buffer->backwards)
    ? buffer->forwards
    : buffer->backwards;
  buffer->newest = rowListNewer(thisNewest, buffer->newest)
    ? thisNewest
    : buffer->newest;
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
  /* RowList *forwards = rowListCons(first, */
  /*   rowListCons(second, */
  /*   rowListCons(third, */
  /*   rowListCons(fourth, */
  /*   rowListCons(fifth, */
  /*   rowListCons(sixth, */
  /*   rowListCons(seventh, NULL))))))); */
  ZipperBuffer *buffer = malloc(sizeof(ZipperBuffer));
  buffer->forwards = NULL;
  buffer->backwards = NULL;
  buffer->newest = NULL;
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
      zipperForwardRow(buffer);
    }
    while (buffer->backwards != NULL) {
      zipperBackwardRow(buffer);
    }
  }
}
