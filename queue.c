#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct queue {
    size_t entry_size;
    int depth;
    unsigned char * buffer;
    int head;
    int tail;
    sem_t sem_space;    // has space in queue
    sem_t sem_work;     // has work in queue
    pthread_mutex_t lock;
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
    
    rc = pthread_mutex_init(&q->lock, NULL);
    if (rc) {
        printf("Error initializing mutex. Error=%d.\n", rc);
        free(q);
        free(q->buffer);
    }
    
    return q;
}

void
queue_cleanup(struct queue * q) {
    if (!q)
        return;

    sem_destroy(&q->sem_work);
    sem_destroy(&q->sem_space);
    free(q->buffer);
    free(q);
}

void
queue_put(struct queue * q, void * entry) {
    sem_wait(&q->sem_space);
    pthread_mutex_lock(&q->lock);

    memcpy(&q->buffer[q->entry_size * q->tail], entry, q->entry_size);
    q->tail = (q->tail + 1) % q->depth;

    pthread_mutex_unlock(&q->lock);
    sem_post(&q->sem_work);
}

void
queue_get(struct queue * q, void * entry) {

    sem_wait(&q->sem_work);
    pthread_mutex_lock(&q->lock);

    memcpy(entry, &q->buffer[q->entry_size * q->head], q->entry_size);
    memset(&q->buffer[q->entry_size * q->head], 0, q->entry_size);
    q->head = (q->head + 1) % q->depth;

    pthread_mutex_unlock(&q->lock);
    sem_post(&q->sem_space);
}

int
queue_timed_get(struct queue * q, void * entry, const struct timespec * timeout) {

    int rc = 0;

    rc = sem_timedwait(&q->sem_work, timeout);
    if (rc) {
        return rc;
    }

    pthread_mutex_lock(&q->lock);

    memcpy(entry, &q->buffer[q->entry_size * q->head], q->entry_size);
    memset(&q->buffer[q->entry_size * q->head], 0, q->entry_size);
    q->head = (q->head + 1) % q->depth;

    pthread_mutex_unlock(&q->lock);
    sem_post(&q->sem_space);
}
