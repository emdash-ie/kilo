#ifndef ZIPPER_BUFFER
#define ZIPPER_BUFFER

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "editorRow.h"

typedef struct RowList RowList;

struct RowList {
  EditorRow *head;
  RowList *tail;
  int number;
};

RowList *rowListCons(EditorRow *head, RowList *tail);

RowList *rowListDrop(RowList *list, int n);

/**
 * True if x is newer than y, false otherwise.
 */
bool rowListNewer(RowList *x, RowList *y);

void updateRowList(RowList *toUpdate, EditorRow *newHead, RowList *newTail);

/**
 * Reverses a RowList by mutating it. Returns the new head.
 */
RowList *rowListReverse(RowList *rows);

typedef struct ZipperBuffer ZipperBuffer;

struct ZipperBuffer {
  RowList *forwards;
  RowList *backwards;
  RowList *newest;
};

void zipperForwardRow(ZipperBuffer *buffer);

void zipperForwardN(ZipperBuffer *buffer, int n);

void zipperBackwardRow(ZipperBuffer *buffer);

void zipperBackwardN(ZipperBuffer *buffer, int n);

void zipperInsertRow(ZipperBuffer *buffer, EditorRow *new);

RowList *zipperRowsFrom(ZipperBuffer *buffer, int cursorY, int n);

void zipperUpdateNewest(ZipperBuffer *buffer);

void printRowList(RowList *list);

void printZipperBuffer(ZipperBuffer *buffer);

ZipperBuffer *exampleBuffer();

int testZipperBuffer();

#endif
