#include <stdio.h>
#include "Point.h"
#include "ListPoint.h"

void printPoint(Point *p) {
  printf("(%d, %d)", p->x, p->y);
}

void printPoints(List(Point) *points) {
  while (points != NULL) {
    printPoint(points->head);
    printf("->");
    points = points->tail;
  }
  printf("|\n");
}

int main() {
  Point p1 = {1, 2};
  Point p2 = {3, 4};
  List(Point) *list1 = ListF(Point).cons(&p1, NULL);
  List(Point) *list2 = ListF(Point).cons(&p2, list1);

  printf("list1: ");
  printPoints(list1);

  printf("list2: ");
  printPoints(list2);
}
