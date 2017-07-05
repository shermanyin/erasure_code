#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct queue {
    size_t entry_size;
    int depth;
    uint8_t * buffer;
    int head;
    int tail;
    sem_t sem_space;    // has space in queue
    sem_t sem_work;     // has work in queue
};

struct queue *
queue_init(size_t entry_size, int depth) {
    int rc = 0;

    struct queue * q = malloc(sizeof(*q));
    if (!q) {
        printf("Error allocating memory for queue.\n");
        return 0;
    }

    q->entry_size = entry_size;
    q->depth = depth;

    q->buffer = malloc(entry_size * depth);
    if (!q->buffer) {
        printf("Error allocating memory for queue.\n");
        free(q);
        return 0;
    }

    memset(q->buffer, 0, entry_size * depth);
    
    q->head = 0;
    q->tail = 0;

    rc = sem_init(&q->sem_space, 0, depth);
    if (rc) {
        printf("Error initializing semaphore.\n");
        free(q);
        free(q->buffer);
        return 0;
    }

    rc = sem_init(&q->sem_work, 0, 0);
    if (rc) {
        printf("Error initializing semaphore.\n");
        free(q);
        free(q->buffer);
        return 0;
    }
    
    return q;
}

void
queue_cleanup(struct queue * q) {
    sem_destroy(&q->sem_work);
    sem_destroy(&q->sem_space);
    free(q->buffer);
    free(q);
}

void
queue_put(struct queue * q, uint8_t * entry) {
    sem_wait(&q->sem_space);

    memcpy(&q->buffer[q->entry_size * q->tail], entry, q->entry_size);

    q->tail = (q->tail + 1) % q->depth;

    sem_post(&q->sem_work);
}

void
queue_get(struct queue * q, uint8_t * entry) {

    sem_wait(&q->sem_work);

    memcpy(entry, &q->buffer[q->entry_size * q->head], q->entry_size);
    memset(&q->buffer[q->entry_size * q->head], 0, q->entry_size);

    q->head = (q->head + 1) % q->depth;

    sem_post(&q->sem_space);
}
