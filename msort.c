#include <stdlib.h>

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