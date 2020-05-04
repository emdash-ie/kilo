#include "linkedList.h"
#include <stdio.h>

typedef struct Point {
  int x;
  int y;
} Point;

typedef struct X {
  int x;
} X;

DeclareList(Point);
DefineList(Point);
DeclareList(List(Point));
DefineList(List(Point));
DeclareList(X);
DefineList(X);
DeclareListFunctions2(Point, X);
DefineListFunctions2(Point, X);
DeclareListFunctions3(Point, Point, Point);
DefineListFunctions3(Point, Point, Point);

X *xProject(Point *p) {
  X *x = malloc(sizeof(X));
  x->x = p->x;
  return x;
}

Point *sum(Point *x, Point *y) {
  Point *p = malloc(sizeof(Point));
  p->x = x->x + y->x;
  return p;
}

int main() {
  Point p1 = {1, 2};
  Point p2 = {3, 4};
  List(Point) *list1 = ListF(Point).cons(&p1, NULL);
  List(Point) *list2 = ListF(Point).cons(&p2, list1);
  printf("Length: %d\n", ListF(Point).length(list2));
  List(List(Point)) *ps = ListF(List(Point)).cons(
    list1,
    ListF(List(Point)).cons(list2, NULL)
  );
  printf("Nested length: %d\n", ListF(List(Point)).length(ps));

  List(List(Point)) *reversed = ListF(List(Point)).reverse(ps);
  printf("x . head . head . reverse $ ps: %d\n", reversed->head->head->x);

  List(X) *list3 = ListF2(Point, X).map(xProject, list2);

  List(Point) *summed = ListF3(Point, Point, Point).zipWith(sum, list1, list2);
}
