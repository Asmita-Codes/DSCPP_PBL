#ifndef ZOMBIE_QUEUE_H
#define ZOMBIE_QUEUE_H
#include <stddef.h>
typedef struct { int *data; size_t capacity, head, size; } ZombieQueue;
int zq_init(ZombieQueue *q, size_t capacity);
void zq_destroy(ZombieQueue *q);
int zq_push(ZombieQueue *q, int value);
int zq_pop(ZombieQueue *q, int *value);
int zq_peek(const ZombieQueue *q, int *value);
size_t zq_size(const ZombieQueue *q);
#endif
