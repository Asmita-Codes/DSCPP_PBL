#include "action_stack.h"
#include <stdlib.h>
#include <string.h>
int as_init(ActionStack*s,size_t c){if(!s||!c)return 0;s->items=(char**)calloc(c,sizeof(char*));if(!s->items)return 0;s->size=0;s->capacity=c;return 1;}
void as_destroy(ActionStack*s){if(s){for(size_t i=0;i<s->size;i++)free(s->items[i]);free(s->items);s->items=0;s->size=s->capacity=0;}}
int as_push(ActionStack*s,const char*m){if(!s||!m||s->size>=s->capacity)return 0;size_t n=strlen(m)+1;s->items[s->size]=(char*)malloc(n);if(!s->items[s->size])return 0;memcpy(s->items[s->size],m,n);s->size++;return 1;}
const char*as_top(const ActionStack*s){return s&&s->size?s->items[s->size-1]:"";}
int as_pop(ActionStack*s,char*out,size_t n){if(!s||!s->size)return 0;char*p=s->items[--s->size];if(out&&n){strncpy(out,p,n-1);out[n-1]=0;}free(p);return 1;}
