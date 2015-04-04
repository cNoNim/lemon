#ifndef _LEMON_MSORT_H_
#define _LEMON_MSORT_H_

/*
 * A generic merge-sort program.
 *
 * USAGE:
 * Let "ptr" be a pointer to some structure which is at the head of
 * a null-terminated list.  Then to sort the list call:
 *
 *     ptr = msort(ptr,&(ptr->next),cmpfnc);
 *
 * In the above, "cmpfnc" is a pointer to a function which compares
 * two instances of the structure and returns an integer, as in
 * strcmp.  The second argument is a pointer to the pointer to the
 * second element of the linked list.  This address is used to compute
 * the offset to the "next" field within the structure.  The offset to
 * the "next" field must be constant for all structures in the list.
 *
 * The function returns a new pointer which is the head of the list
 * after sorting.
 *
 * ALGORITHM:
 * Merge-sort.
 */

char *msort(char *, char **, int (*)(const char *, const char *));
void *mergesort(void *list, void *(*get_next_fn)(void const *), void (*set_next_fn)(void *, void *),
                int (*compare_fn)(void const *, void const *));

#endif //_LEMON_MSORT_H_
