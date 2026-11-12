#include "inventory.h"
#include <stdlib.h>
#include <string.h>
void inv_clear(InventoryNode**h){if(!h)return;while(*h){InventoryNode*n=(*h)->next;free(*h);*h=n;}}
static InventoryNode*find(InventoryNode*h,const char*n){for(;h;h=h->next)if(strcmp(h->name,n)==0)return h;return 0;}
int inv_set(InventoryNode**h,const char*n,int c){if(!h||!n)return 0;InventoryNode*p=find(*h,n);if(p){p->count=c;return 1;}p=(InventoryNode*)calloc(1,sizeof(*p));if(!p)return 0;strncpy(p->name,n,sizeof(p->name)-1);p->count=c;p->next=*h;*h=p;return 1;}
int inv_add(InventoryNode**h,const char*n,int d){int c=inv_get(h?*h:0,n);return inv_set(h,n,c+d);}
int inv_get(const InventoryNode*h,const char*n){for(;h;h=h->next)if(strcmp(h->name,n)==0)return h->count;return 0;}
int inv_remove_one(InventoryNode**h,const char*n){if(!h)return 0;InventoryNode**p=h;while(*p&&strcmp((*p)->name,n))p=&(*p)->next;if(!*p||(*p)->count<=0)return 0;if(--(*p)->count==0){InventoryNode*t=*p;*p=t->next;free(t);}return 1;}
