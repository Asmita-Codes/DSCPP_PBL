#ifndef ACTION_STACK_H
#define ACTION_STACK_H
#include <stddef.h>
typedef struct { char **items; size_t size, capacity; } ActionStack;
int as_init(ActionStack*s,size_t cap);void as_destroy(ActionStack*s);int as_push(ActionStack*s,const char*msg);const char*as_top(const ActionStack*s);int as_pop(ActionStack*s,char*out,size_t n);
#endif
