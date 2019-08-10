#ifndef ZIPPER_BUFFER
#define ZIPPER_BUFFER

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "editorRow.c"

typedef struct Row Row;

struct Row {
  char *characters;
  int length;
};

Row *newRow(char *characters, int length) {
  Row *row = malloc(sizeof(Row));
  row->characters = characters;
  row->length = length;
  return row;
}

typedef struct RowList RowList;

struct RowList {
  Row *head;
  RowList *tail;
  int number;
};

int rowListId = 0;

RowList *newRowList(Row *head, RowList *tail) {
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

void updateRowList(RowList *toUpdate, Row *newHead, RowList *newTail) {
  if (toUpdate == NULL) return;
  toUpdate->head = newHead;
  toUpdate->tail = newTail;
  toUpdate->number = rowListId++;
}

typedef struct ZipperBuffer ZipperBuffer;

struct ZipperBuffer {
  RowList *forwards;
  RowList *backwards;
};

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

void printRowList(RowList *list) {
  int i = 1;
  while (list != NULL) {
    printf("%d: %s\n", i, list->head->characters);
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
  Row *first = newRow("That's great, it starts with an earthquake.", 0);
  Row *second = newRow("Birds and snakes, an aeroplane.", 0);
  Row *third = newRow("Lenny Bruce is not afraid.", 0);
  Row *fourth = newRow("Eye of a hurricane, listen to yourself churn,", 0);
  Row *fifth = newRow("World serves its own needs, dummy serve your own needs", 0);
  Row *sixth = newRow("Feed it off an aux speak, grunt no strength", 0);
  Row *seventh = newRow("The ladder starts to clatter with fear of fight, down height", 0);
  RowList *forwards = newRowList(first,
    newRowList(second,
    newRowList(third,
    newRowList(fourth,
    newRowList(fifth,
    newRowList(sixth,
    newRowList(seventh, NULL)))))));
  ZipperBuffer *buffer = malloc(sizeof(ZipperBuffer));
  buffer->forwards = forwards;
  buffer->backwards = NULL;
  return buffer;
}

int main() {
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

#endif
