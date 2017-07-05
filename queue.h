#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

struct queue;
    
struct queue * queue_init(size_t entry_size, int depth);

void queue_cleanup(struct queue * q);

void queue_put(struct queue * q, uint8_t entry);

void queue_get(struct queue * q, uint8_t entry);

#endif /* QUEUE_H */
