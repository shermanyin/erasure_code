#ifndef QUEUE_H
#define QUEUE_H

struct queue;
    
struct queue * queue_init(size_t entry_size, int depth);

void queue_cleanup(struct queue * q);

void queue_put(struct queue * q, void * entry);

void queue_get(struct queue * q, void * entry);

int queue_timed_get(struct queue * q, void * entry, const struct timespec * timeout);

#endif /* QUEUE_H */
