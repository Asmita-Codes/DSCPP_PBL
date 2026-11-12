#include "zombie_queue.h"
#include <stdlib.h>
int zq_init(ZombieQueue*q,size_t c){
    if(!q||!c)
    return 0;
    q->data=(int*)malloc(c*sizeof(int));
    if(!q->data)return 0;q->capacity=c;
    q->head=q->size=0;return 1;
}
void zq_destroy(ZombieQueue*q){
    if(q){free(q->data);
        q->data=0;q->capacity=q->head=q->size=0;
    }
}
int zq_push(ZombieQueue*q,int v)
{
    if(!q||!q->data||q->size==q->capacity)
    return 0;
q->data[(q->head+q->size)%q->capacity]=v;
q->size++;return 1;
}
int zq_pop(ZombieQueue*q,int*v){
    if(!q||!q->size)return 0;
    if(v)*v=q->data[q->head];
    q->head=(q->head+1)%q->capacity;
    q->size--;return 1;
}
int zq_peek(const ZombieQueue*q,int*v){
    if(!q||!q->size)return 0;if(v)*v=q->data[q->head];
    return 1;
}
size_t zq_size(const ZombieQueue*q){
    return q?q->size:0;
}
