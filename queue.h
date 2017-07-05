#ifndef QUEUE_H
#define QUEUE_H

struct queue;
    
struct queue * queue_init(size_t entry_size, int depth);

void queue_cleanup(struct queue * q);

void queue_put(struct queue * q, int * entry);

void queue_get(struct queue * q, int * entry);

#endif /* QUEUE_H */
