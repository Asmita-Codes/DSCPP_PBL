#include "priority_queue.h"
#include <stdlib.h>
static void sw(PQNode*a,PQNode*b){PQNode t=*a;*a=*b;*b=t;}
int pq_init(PriorityQueue*q,size_t c){if(!q||!c)return 0;q->a=(PQNode*)malloc(c*sizeof(PQNode));if(!q->a)return 0;q->size=0;q->capacity=c;return 1;}
void pq_destroy(PriorityQueue*q){if(q){free(q->a);q->a=0;q->size=q->capacity=0;}}
int pq_push(PriorityQueue*q,float p,int v){if(!q||q->size>=q->capacity)return 0;size_t i=q->size++;q->a[i]=(PQNode){p,v};while(i){size_t par=(i-1)/2;if(q->a[par].priority<=q->a[i].priority)break;sw(&q->a[par],&q->a[i]);i=par;}return 1;}
int pq_pop(PriorityQueue*q,PQNode*out){if(!q||!q->size)return 0;if(out)*out=q->a[0];q->a[0]=q->a[--q->size];size_t i=0;for(;;){size_t l=i*2+1,r=l+1,m=i;if(l<q->size&&q->a[l].priority<q->a[m].priority)m=l;if(r<q->size&&q->a[r].priority<q->a[m].priority)m=r;if(m==i)break;sw(&q->a[m],&q->a[i]);i=m;}return 1;}
int pq_peek(const PriorityQueue*q,PQNode*out){if(!q||!q->size)return 0;if(out)*out=q->a[0];return 1;}
