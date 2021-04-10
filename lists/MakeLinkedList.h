#if n == 1

#ifdef a

#include <stdlib.h>

#define _List(a) List__##a
#define List(a) _List(a)
#define _ListF(a) listFunctions__##a
#define ListF(a) _ListF(a)
#define _ListFT(a) ListFunctions__##a
#define ListFT(a) _ListFT(a)


typedef struct List(a) {
    a *head;
    struct List(a) *tail;
} List(a);

#define _ListConsName(a) listCons__ ## a
#define ListConsName(a) _ListConsName(a)

List(a) *ListConsName(a)(a *head, List(a) *tail);

#define _ListLengthName(a) listLength__ ## a
#define ListLengthName(a) _ListLengthName(a)

int ListLengthName(a)(List(a) *list);

#define _ListReverseName(a) listReverse__ ## a
#define ListReverseName(a) _ListReverseName(a)

List(a) *ListReverseName(a)(List(a) *list);

#define _ListConcatName(a) listConcat__ ## a
#define ListConcatName(a) _ListConcatName(a)

List(a) *ListConcatName(a)(List(a) *first, List(a) *second);

#define _ListFreeName(a) listFree__ ## a
#define ListFreeName(a) _ListFreeName(a)

void ListFreeName(a)(List(a) *as);

typedef struct ListFT(a) {
    List(a) * (*cons)(a *, List(a) *);
    int (*length)(List(a) *);
    List(a) * (*reverse)(List(a) *);
    List(a) * (*concat)(List(a) *, List(a) *);
    void (*free)(List(a) *);
} ListFT(a);

extern ListFT(a) ListF(a);

#ifdef LinkedListImplementation

List(a) *ListConsName(a)(a *head, List(a) *tail) {
    List(a) *list = malloc(sizeof(List(a)));
    list->head = head;
    list->tail = tail;
    return list;
}

int ListLengthName(a)(List(a) *list) {
    if (list == NULL) {
        return 0;
    } else {
        return 1 + ListLengthName(a)(list->tail);
    }
}

List(a) *ListReverseName(a)(List(a) *list) {
    List(a) *reversed = NULL;
    while (list != NULL) {
        reversed = ListConsName(a)(list->head, reversed);
        list = list->tail;
    }
    return reversed;
}

List(a) *ListConcatName(a)(List(a) *first, List(a) *second) {
    if (first == NULL) {
        return second;
    } else {
        return ListConsName(a)(first->head, ListConcatName(a)(first->tail, second));
    }
}

void ListFreeName(a)(List(a) *as) {
    if (as != NULL) {
        List(a) *next = as->tail;
        free(as);
        ListFreeName(a)(next);
    }
}

ListFT(a) ListF(a) = {
    ListConsName(a),
    ListLengthName(a),
    ListReverseName(a),
    ListConcatName(a),
    ListFreeName(a)
};

#endif // LinkedListImplementation

#undef a

#endif // a

#undef n

#endif // n == 1

#if n == 2

#if defined a && defined b

#define _ListF2(a, b) listFunctions2__##a##b
#define ListF2(a, b) _ListF2(a, b)
#define ListF2T(a, b) ListFunctions__##a##b

#define _ListMapName(a, b) listMap__ ## a ## b
#define ListMapName(a, b) _ListMapName(a, b)

List(b) *ListMapName(a, b)(b *(*f)(a *), List(a) *list);

#define _ListFoldrName(a, b) listFoldr__ ## a ## b
#define ListFoldrName(a, b) _ListFoldrName(a, b)

b *ListFoldrName(a, b)(b *(*f)(a *, b *), b *z, List(a) *as);

#ifdef LinkedListImplementation

List(b) *ListMapName(a, b)(b *(*f)(a *), List(a) *list) {
    if (list == NULL) {
        return NULL;
    } else {
        return ListF(b).cons(f(list->head), ListMapName(a, b)(f, list->tail));
    }
}

b *ListFoldrName(a, b)(b *(*f)(a *, b *), b *z, List(a) *as) {
    if (as == NULL) {
        return z;
    } else {
        return f(as->head, ListFoldrName(a, b)(f, z, as->tail));
    }
}

typedef struct ListF2T(a, b) {
    List(b) * (*map)(b * (*f)(a *), List(a) *);
    b *(*foldr)(b * (*f)(a *, b *), b *, List(a) *);
} ListF2T(a, b);

ListF2T(a, b) ListF2(a, b) = {ListMapName(a, b), ListFoldrName(a, b)};

#endif // LinkedListImplementation

#undef a
#undef b

#else // a && b
#error "LinkedList included with n == 2, but either a or b is not defined."
#endif // a && b

#undef n

#endif // n == 2

#if n == 3

#if defined a && defined b && defined c

#define _ListF3(a, b, c) listFunctions3__##a##b##c
#define ListF3(a, b, c) _ListF3(a, b, c)
#define _ListF3T(a, b, c) ListFunctions##a##b##c
#define ListF3T(a, b, c) _ListF3T(a, b, c)

#define _ListZipWithName(a, b, c) listZipWith__ ## a ## b ## c
#define ListZipWithName(a, b, c) _ListZipWithName(a, b, c)

List(c) * ListZipWithName(a, b, c)(c *(*f)(a *, b *), List(a) *as, List(b) *bs);

#ifdef LinkedListImplementation

List(c) * ListZipWithName(a, b, c)(c *(*f)(a *, b *), List(a) *as, List(b) *bs) {
    if (as == NULL || bs == NULL) {
        return NULL;
    } else {
        return ListConsName(c)(
            f(as->head, bs->head),
            ListZipWithName(a, b, c)(f, as->tail, bs->tail)
        );
    }
}

typedef struct ListF3T(a, b, c) {
    List(c) *(*zipWith)(c *(*f)(a *, b *), List(a) *, List(b) *);
} ListF3T(a, b, c);

ListF3T(a, b, c) ListF3(a, b, c) = {ListZipWithName(a, b, c)};

#endif // LinkedListImplementation

#undef a
#undef b
#undef c

#else // a && b && c
#error "LinkedList included with n == 3, but either a, b or c is not defined."
#endif // a && b && c

#undef n

#endif // n == 3

#if n == 4

#if defined a && defined b && defined c && defined d

#define _ListF4(a, b, c, d) listFunctions4__##a##b##c##d
#define ListF4(a, b, c, d) _ListF4(a, b, c, d)
#define _ListF4T(a, b, c, d) ListFunctions##a##b##c##d
#define ListF4T(a, b, c, d) _ListF4T(a, b, c, d)

#define _ListZipWith3Name(a, b, c, d) listZipWith3__ ## a ## b ## c ## d
#define ListZipWith3Name(a, b, c, d) _ListZipWith3Name(a, b, c, d)

typedef struct ListF4T(a, b, c, d) {
    List(d) *(*zipWith)(d *(*f)(a *, b *, c *), List(a) *, List(b) *, List(c) *);
} ListF4T(a, b, c, d);

extern ListF4T(a, b, c, d) ListF4(a, b, c, d);

#ifdef LinkedListImplementation

List(d) *ListZipWith3Name(a, b, c, d)(d *(*f)(a *, b *, c *), List(a) *as, List(b) *bs, List(c) *cs) {
    if (as == NULL || bs == NULL || cs == NULL) {
        return NULL;
    } else {
        return ListConsName(d)(f(as->head, bs->head, cs->head),
                             ListZipWith3Name(a, b, c, d)(f, as->tail, bs->tail, cs->tail));
    }
}

ListF4T(a, b, c, d) ListF4(a, b, c, d) = {ListZipWith3Name(a, b, c, d)};

#endif // LinkedListImplementation

#undef a
#undef b
#undef c
#undef d

#else // a && b && c && d
#error "LinkedList included with n == 4, but either a, b, c, or d is not defined."
#endif // a && b && c && d

#undef n

#endif // n == 4
