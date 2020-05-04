#pragma once
#include <stdlib.h>

#define _List(a) List__##a
#define List(a) _List(a)
#define _ListF(a) listFunctions__##a
#define ListF(a) _ListF(a)
#define ListFT(a) ListFunctions__##a

#define _ListF2(a, b) listFunctions2__##a##b
#define ListF2(a, b) _ListF2(a, b)
#define ListF2T(a, b) ListFunctions__##a##b
#define _ListF3(a, b, c) listFunctions3__##a##b##c
#define ListF3(a, b, c) _ListF3(a, b, c)
#define ListF3T(a, b, c) ListFunctions##a##b##c
#define _ListF4(a, b, c, d) listFunctions4__##a##b##c##d
#define ListF4(a, b, c, d) _ListF4(a, b, c, d)
#define ListF4T(a, b, c, d) ListFunctions##a##b##c##d

#define DeclareList(a) DefineListStruct(a) DeclareListFunctions(a)

#define DefineList(a) DefineListFunctions(a)

#define DefineListStruct(a)                                                      \
  typedef struct List(a) {                                                     \
    a *head;                                                                   \
    struct List(a) *tail;                                                     \
  }                                                                            \
  List(a);

#define DeclareListFunctions(a)                 \
  DeclareCons(a)                                \
  DeclareLength(a)                            \
  DeclareReverse(a)                           \
  DeclareConcat(a)

#define DefineListFunctions(a)                                                   \
  DefineCons(a)                                                                  \
  DefineLength(a)                                                                \
  DefineReverse(a)                                                               \
  DefineConcat(a)                                                                \
  DefineListFunctionStruct(a)

#define DeclareCons(a)                          \
  List(a) *listCons__##a(a *head, List(a) *tail);

#define DefineCons(a)                                                            \
  List(a) *listCons__##a(a *head, List(a) *tail) {                           \
    List(a) *list = malloc(sizeof(List(a)));                                   \
    list->head = head;                                                         \
    list->tail = tail;                                                         \
    return list;                                                               \
  }

#define DeclareLength(a) \
  int listLength__##a(List(a) *list);          \

#define DefineLength(a)                                                          \
  int listLength__##a(List(a) *list) {                                        \
    if (list == NULL) {                                                        \
      return 0;                                                                \
    } else {                                                                   \
      return 1 + listLength__##a(list->tail);                                  \
    }                                                                          \
  }

#define DeclareReverse(a)                                                         \
  List(a) *listReverse__##a(List(a) *list);                            \

#define DefineReverse(a)                                                         \
  List(a) *listReverse__##a(List(a) *list) {                                 \
    List(a) *reversed = NULL;                                                  \
    while (list != NULL) {                                                     \
      reversed = listCons__##a(list->head, reversed);                          \
      list = list->tail;                                                       \
    }                                                                          \
    return reversed;                                                           \
  }

#define DeclareConcat(a)                                                   \
  List(a) *listConcat__##a(List(a) *first, List(a) *second);         \

#define DefineConcat(a)                                                          \
  List(a) *listConcat__##a(List(a) *first, List(a) *second) {               \
    if (first == NULL) {                                                       \
      return second;                                                           \
    } else {                                                                   \
      return listCons__##a(first->head, listConcat__##a(first->tail, second)); \
    }                                                                          \
  }

#define DefineListFunctionStruct(a)                                            \
  typedef struct ListFT(a) {                                                   \
    List(a) * (*cons)(a *, List(a) *);                                         \
    int (*length)(List(a) *);                                                  \
    List(a) * (*reverse)(List(a) *);                                           \
    List(a) * (*concat)(List(a) *, List(a) *);                                 \
  } ListFT(a);                                                                 \
  ;                                                                            \
  ListFT(a) ListF(a) = {listCons__##a, listLength__##a, listReverse__##a,      \
                        listConcat__##a};

#define DeclareListFunctions2(a, b) \
  DeclareMap(a, b) \
  DeclareFoldr(a, b) \

#define DefineListFunctions2(a, b) \
  DefineMap(a, b) \
  DefineFoldr(a, b) \
  DefineListFunction2Struct(a, b)

#define DeclareMap(a, b) \
  List(b) *listMap__##a##b(b *(*f)(a *), List(a) *list);

#define DefineMap(a, b)                                                          \
  List(b) *listMap__##a##b(b *(*f)(a *), List(a) *list) {                    \
    if (list == NULL) {                                                        \
      return NULL;                                                             \
    } else {                                                                   \
      return ListF(b).cons(f(list->head), listMap__##a##b(f, list->tail));     \
    }                                                                          \
  }

#define DeclareFoldr(a, b)                                                        \
  b *listFoldr__##a##b(b *(*f)(a *, b *), b *z, List(a) *as);

#define DefineFoldr(a, b)                                                        \
  b *listFoldr__##a##b(b *(*f)(a *, b *), b *z, List(a) *as) {                \
    if (as == NULL) {                                                          \
      return z;                                                                \
    } else {                                                                   \
      return f(as->head, listFoldr__##a##b(f, z, as->tail));                   \
    }                                                                          \
  }

#define DefineListFunction2Struct(a, b) typedef struct ListF2T(a, b) {     \
      List(b) * (*map)(b * (*f)(a *), List(a) *);                              \
      b *(*foldr)(b * (*f)(a *, b *), b *, List(a) *);                         \
    }                                                                          \
    ListF2T(a, b);                                                             \
    ListF2T(a, b) ListF2(a, b) = {listMap__##a##b, listFoldr__##a##b};

#define DeclareListFunctions3(a, b, c)                                          \
  DeclareZipWith(a, b, c)

#define DefineListFunctions3(a, b, c)                                          \
  DefineZipWith(a, b, c) \
  DefineListFunction3Struct(a, b, c)

#define DeclareZipWith(a, b, c)                                                   \
  List(c) *listZipWith__##a##b##c(c *(*f)(a *, b *), List(a) *as, List(b) *bs);

#define DefineZipWith(a, b, c)                                                   \
  List(c) *listZipWith__##a##b##c(c *(*f)(a *, b *), List(a) *as, List(b) *bs) { \
    if (as == NULL || bs == NULL) {                                            \
      return NULL;                                                             \
    } else {                                                                   \
      return listCons__##c(f(as->head, bs->head),                              \
                           listZipWith__##a##b##c(f, as->tail, bs->tail)); \
    }                                                                          \
  }

#define DefineListFunction3Struct(a, b, c)                                       \
  typedef struct ListF3T(a, b, c) {                                            \
    List(c) *(*zipWith)(c *(*f)(a *, b *), List(a) *, List(b) *);    \
  }                                                                            \
  ListF3T(a, b, c);                                                            \
  ListF3T(a, b, c) ListF3(a, b, c) = {listZipWith__##a##b##c};

#define DeclareListFunctions4(a, b, c, d)                                       \
  DeclareZipWith3(a, b, c, d)

#define DefineListFunctions4(a, b, c, d)                                       \
  DefineZipWith3(a, b, c, d) DefineListFunction4Struct(a, b, c, d)

#define DeclareZipWith3(a, b, c, d)                                          \
  List(d) *listZipWith__##a##b##c##d(d *(*f)(a *, b *, c *), List(a) *as, List(b) *bs, List(c) *cs);

#define DefineZipWith3(a, b, c, d)                                          \
  List(d) *listZipWith__##a##b##c##d(d *(*f)(a *, b *, c *), List(a) *as, List(b) *bs, List(c) *cs) { \
    if (as == NULL || bs == NULL || cs == NULL) {                                            \
      return NULL;                                                             \
    } else {                                                                   \
      return listCons__##d(f(as->head, bs->head, cs->head),                      \
                           listZipWith__##a##b##c##d(f, as->tail, bs->tail, cs->tail)); \
    }                                                                          \
  }

#define DefineListFunction4Struct(a, b, c, d)                               \
  typedef struct ListF4T(a, b, c, d) {                                    \
    List(d) *(*zipWith)(d *(*f)(a *, b *, c *), List(a) *, List(b) *, List(c) *); \
  }                                                                            \
  ListF4T(a, b, c, d);                                                    \
  ListF4T(a, b, c, d) ListF4(a, b, c, d) = {listZipWith__##a##b##c##d};
