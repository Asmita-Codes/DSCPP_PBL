#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H
#include <stddef.h>
typedef struct { float priority; int value; } PQNode;
typedef struct { PQNode *a; size_t size, capacity; } PriorityQueue;
int pq_init(PriorityQueue*q,size_t cap); void pq_destroy(PriorityQueue*q); int pq_push(PriorityQueue*q,float p,int v); int pq_pop(PriorityQueue*q,PQNode*out); int pq_peek(const PriorityQueue*q,PQNode*out);
#endif
