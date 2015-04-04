#include <stdlib.h>

/* Return a pointer to the next structure in the linked list.
 */
#define LISTSIZE 30
#define NEXT(A) (*(char **)(((char *)A) + offset))

/* Inputs:
 *   a:       A sorted, null-terminated linked list.  (May be null).
 *   b:       A sorted, null-terminated linked list.  (May be null).
 *   cmp:     A pointer to the comparison function.
 *   offset:  Offset in the structure to the "next" field.
 *
 * Return Value:
 *   A pointer to the head of a sorted list containing the elements
 *   of both a and b.
 *
 * Side effects:
 *   The "next" pointers for elements in the lists a and b are
 *   changed.
 */
static char *
merge(char *a, char *b, int (*cmp)(const char *, const char *), size_t offset) {
  char *ptr, *head;

  if (a == 0) {
    head = b;
  } else if (b == 0) {
    head = a;
  } else {
    if ((*cmp)(a, b) <= 0) {
      ptr = a;
      a = NEXT(a);
    } else {
      ptr = b;
      b = NEXT(b);
    }
    head = ptr;
    while (a && b) {
      if ((*cmp)(a, b) <= 0) {
        NEXT(ptr) = a;
        ptr = a;
        a = NEXT(a);
      } else {
        NEXT(ptr) = b;
        ptr = b;
        b = NEXT(b);
      }
    }
    if (a)
      NEXT(ptr) = a;
    else
      NEXT(ptr) = b;
  }
  return head;
}

/* Inputs:
 *   list:      Pointer to a singly-linked list of structures.
 *   next:      Pointer to pointer to the second element of the list.
 *   cmp:       A comparison function.
 *
 * Return Value:
 *   A pointer to the head of a sorted list containing the elements
 *   orginally in list.
 *
 * Side effects:
 *   The "next" pointers for elements in list are changed.
 */
char *
msort(char *list, char **next, int (*cmp)(const char *, const char *)) {
  size_t offset;
  char *ep;
  char *set[LISTSIZE];
  int i;
  offset = (size_t)next - (size_t)list;
  for (i = 0; i < LISTSIZE; i++)
    set[i] = 0;
  while (list) {
    ep = list;
    list = NEXT(list);
    NEXT(ep) = 0;
    for (i = 0; i < LISTSIZE - 1 && set[i] != 0; i++) {
      ep = merge(ep, set[i], cmp, offset);
      set[i] = 0;
    }
    set[i] = ep;
  }
  ep = 0;
  for (i = 0; i < LISTSIZE; i++)
    if (set[i])
      ep = merge(set[i], ep, cmp, offset);
  return ep;
}

struct mergesort_sublist {
  void *list;
  unsigned long length;
};

static void *get_nth_next(void *list, unsigned long n, void *(*get_next_fn)(void const *));
static void *pop_item(struct mergesort_sublist *list, void *(*get_next_fn)(void const *));

void *
mergesort(void *list, void *(*get_next_fn)(void const *), void (*set_next_fn)(void *, void *),
          int (*compare_fn)(void const *, void const *)) {
  unsigned long l;
  if (!list)
    return NULL;
  for (l = 1;; l *= 2) {
    void *current;
    struct mergesort_sublist p, q;

    p.list = list;
    q.list = get_nth_next(p.list, l, get_next_fn);
    if (!q.list)
      break;
    p.length = q.length = l;

    if (compare_fn(p.list, q.list) > 0)
      list = current = pop_item(&q, get_next_fn);
    else
      list = current = pop_item(&p, get_next_fn);

    while (p.list) {
      while (p.length || q.length) {
        void *prev = current;
        if (!p.length)
          current = pop_item(&q, get_next_fn);
        else if (!q.length)
          current = pop_item(&p, get_next_fn);
        else if (compare_fn(p.list, q.list) > 0)
          current = pop_item(&q, get_next_fn);
        else
          current = pop_item(&p, get_next_fn);
        set_next_fn(prev, current);
      }
      p.list = q.list;
      p.length = l;
      q.list = get_nth_next(p.list, l, get_next_fn);
      q.length = q.list ? l : 0;
    }
    set_next_fn(current, NULL);
  }
  return list;
}

static void *
get_nth_next(void *list, unsigned long n, void *(*get_next_fn)(void const *)) {
  while (n-- && list)
    list = get_next_fn(list);
  return list;
}

static void *
pop_item(struct mergesort_sublist *l, void *(*get_next_fn)(void const *)) {
  void *p = l->list;
  l->list = get_next_fn(l->list);
  l->length = l->list ? (l->length - 1) : 0;
  return p;
}