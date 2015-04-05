#include "rule.h"

#include <stdlib.h>

struct rule_list *rule_list_insert(struct rule *rule, struct rule_list **list) {
    struct rule_list *new_list = (struct rule_list *)malloc(sizeof(struct rule_list));
    new_list->item = rule;
    new_list->next = *list;
    *list = new_list;
    return new_list;
}

