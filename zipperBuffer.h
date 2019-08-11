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

RowList *newRowList(EditorRow *head, RowList *tail);

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
};

void zipperForwardRow(ZipperBuffer *buffer, RowList *keepBefore);

void zipperForwardN(ZipperBuffer *buffer, RowList *keepBefore, int n);

void zipperBackwardRow(ZipperBuffer *buffer, RowList *keepBefore);

void zipperBackwardN(ZipperBuffer *buffer, RowList *keepBefore, int n);

void zipperInsertRow(ZipperBuffer *buffer, EditorRow *new);

void printRowList(RowList *list);

void printZipperBuffer(ZipperBuffer *buffer);

ZipperBuffer *exampleBuffer();

int testZipperBuffer();

#endif
