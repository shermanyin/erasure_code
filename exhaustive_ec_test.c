#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "erasure_code.h"
#include "queue.h"

#define PROG_NAME "exhaustive_ec_test"

#define QUEUE_DEPTH (10)
#define QUEUE_TIMEOUT_S (1)

const char * usage = 
"This program tests Erasure Code decoding for all combinations of bytes lost.\n\n"
"usage: " PROG_NAME " k p l\n"
"k: number of randomly generated input bytes\n"
"p: number of parity bytes to generate\n"
"Example: " PROG_NAME " 4 2\n\n";

const char * mem_err = "Error allocating memory.";

/*
 * Might be using more global variables than I should, but for a simple 
 * program, this shortcut is ok.  Most of these are read-only after first set.
 */
struct queue * queue;

uint8_t * ec_code;

int work_done;

struct thread_data {
    uint32_t k;
};

struct thread_data thread_data;

struct results {
    uint64_t passed;
    uint64_t failed;
    pthread_mutex_t lock;
};

struct results res;

void print_array(int * a, size_t len) {
    for (int i = 0; i < len; i++)
        printf("%02x, ", (int)a[i]);
    printf("\n");
}

void print_array8(uint8_t * a, size_t len) {
    for (int i = 0; i < len; i++)
        printf("%02x, ", (int)a[i]);
    printf("\n");
}

/*
 * Generate all combinations in the set n choose r, then run the specified 
 * function on the combination.
 *
 * Each sequence will be in the form of 0-base indices. E.g. 3 choose 2 means
 * pick 2 numbers from [0, 1, 2]: [0, 1], [0, 2], and [1, 2] are all the
 * possible combinations.
 *
 * n (IN):  number of indices to choose from
 * r (IN):  number of indices to pick
 * comb (IN):  int array of length r to hold combinations passed into fcn()
 * pos (IN): position in comb[] to start in
 * start (IN): first index to use in combination
 * counter (OUT): returns number of combinations found
 * fcn (IN): function to call when a combination is found
 * arg (IN): arg to pass to fcn() when called
 */
void combination(int n,
                 int r,
                 int * comb,
                 int pos,
                 int start,
                 uint64_t * counter,
                 void (*fcn)(int *, int, void *),
                 void * arg) {
    if (pos == r) {
        (*fcn)(comb, r, arg);
        (*counter)++;
        return;
    }

    for (int i = start; i <= n - r + pos; i++) {
        comb[pos] = i;
        combination(n, r, comb, pos + 1, i + 1, counter, fcn, arg);
    }
}

void signal_thread(int * array, int len, void * arg) {
#if 0
    printf("Adding to queue: ");
    for (int i = 0; i < len; i++)
        printf("%d ", array[i]);
    printf("\n");
#endif
    queue_put(queue, array);
}

void * decode_one(void * arg) {
    int i = 0;
    int * recv_idx = 0;
    uint8_t * to_decode = 0;
    uint8_t * decoded = 0;

    printf("Starting thread...\n");

    recv_idx = malloc(sizeof(*recv_idx) * thread_data.k);
    if (!recv_idx) {
        printf("%s\n", mem_err);
        goto decode_one_err;
    }

    to_decode = malloc(sizeof(*to_decode) * thread_data.k);
    if (!to_decode) {
        printf("%s\n", mem_err);
        goto decode_one_err;
    }

    decoded = malloc(sizeof(*decoded) * thread_data.k);
    if (!decoded) {
        printf("%s\n", mem_err);
        goto decode_one_err;
    }

    while (1) {
        int rc = 0;
        struct timespec ts;

        rc = clock_gettime(CLOCK_REALTIME, &ts);
        if (rc) {
            printf("Error getting time.\n");
            goto decode_one_err;
        }

        ts.tv_sec += QUEUE_TIMEOUT_S;

        rc = queue_timed_get(queue, recv_idx, &ts);
        if (rc) {
            // Check if work is done
            if (work_done) {
                break;
            } else {
                continue;
            }
        }

        //printf("Received: ");
        for (i = 0; i < thread_data.k; i++) {
            //printf("%d ", recv_idx[i]);
            to_decode[i] = ec_code[recv_idx[i]];
        }
        //printf("\n");

        rc = ec_decode(to_decode, recv_idx, decoded);

        if (rc) {
            printf("Decode failed\n");
        } else {
            // Check decoded data
            for (i = 0; i < thread_data.k; i++) {
                if (ec_code[i] != decoded[i]) {
                    printf("Error: Incorrect decoded data\n");
                    rc = 1;
                    break;
                }
            }
        }

        pthread_mutex_lock(&res.lock);
        if (rc) {
            res.failed++;
        } else {
            res.passed++;
        }
        pthread_mutex_unlock(&res.lock);
    } // while (1)

decode_one_err:
    free(decoded);
    free(to_decode);
    free(recv_idx);
    return 0;
}

void rand_data_gen(uint8_t * data, size_t len) {
    srand(time(NULL));

    for (int i = 0; i < len; i++)
        data[i] = (uint8_t) (rand() % (UINT8_MAX + 1));
}

int main(int argc, char* argv[]) {
    uint32_t k = 0;
    uint32_t p = 0;
    uint64_t counter = 0;
    int * recv_idx = 0;
    int i = 0;
    int num_threads = 0;
    int rc = 0;
    pthread_t * threads = 0;

    if (argc != 3) {
        printf("Requires 2 parameters.\n\n");
        printf("%s\n\n", usage);
        exit(1);
    }

    k = atoi(argv[1]);
    p = atoi(argv[2]);

    // init erasure code module
    rc = ec_init(k, p);
    if (rc) {
        printf("Error initializing Erasure Code.\n");
        exit(1);
    }

    // Generate random data and calculate parity
    ec_code = malloc(sizeof(*ec_code) * (k + p));
    if (!ec_code) {
        printf("%s\n", mem_err);
        rc = -1;
        goto err;
    }

    rand_data_gen(ec_code, k);

    rc = ec_encode(ec_code, ec_code + k);
    if (rc) {
        printf("Error encoding data.\n");
        goto err;
    }

    printf("Original data: ");
    print_array8(ec_code, k);

    printf("Erasure Code:  ");
    print_array8(ec_code, k + p);

    // init queue which has built-in semaphore and mutex
    queue = queue_init(sizeof(int) * k, QUEUE_DEPTH);
    if (!queue) {
        printf("Error initializing queue.\n");
        rc = -1;
        goto err;
    }

    // init results to capture results from threads
    rc = pthread_mutex_init(&res.lock, NULL);
    if (rc) {
        printf("Error initializing mutex. Error=%d.\n", rc);
        goto err;
    }

    // start threads
    num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Starting %d threads\n", num_threads);

    thread_data.k = k;

    work_done = 0;

    threads = malloc(sizeof(*threads) * num_threads);
    if (!threads) {
        printf("%s\n", mem_err);
        rc = -1;
        goto err;
    }

    for (i = 0; i < num_threads; i++) {
        rc = pthread_create(&threads[i], NULL, decode_one, NULL);
        if (rc) {
            printf("Error creating thread. (error=%d)\n", rc);
            goto err;
        }
    }

    // generate combinations and signal to threads
    printf("generate combinations, %d choose %d...\n", k + p, k);
    recv_idx = malloc(sizeof(*recv_idx) * k);
    if (!recv_idx) {
        printf("%s\n", mem_err);
        rc = -1;
        goto err;
    }

    combination(k + p, k, recv_idx, 0, 0, &counter, &signal_thread, NULL);

    printf("%lu combinations found.\n", counter);

    work_done = 1;

    // when we got all results, exit
    while (1) {
        pthread_mutex_lock(&res.lock);
        if (res.passed + res.failed >= counter) {
            printf("Results: %lu of %lu passed.\n",
                   res.passed, res.passed + res.failed);
            break;
        }
        pthread_mutex_unlock(&res.lock);
        printf("Waiting for results...\n");
        sleep(2);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

err:
    // clean up
    free(recv_idx);
    free(threads);
    queue_cleanup(queue);
    free(ec_code);
    ec_cleanup();

    return rc;
}
