#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "erasure_code.h"
#include "queue.h"

#define PROG_NAME "exhaustive_ec_test"
#define QUEUE_DEPTH (2)

struct thread_data {
    uint32_t k;
    uint32_t p;
};

struct results {
    uint64_t passed;
    uint64_t failed;
    pthread_mutex_t lock;
};

const char * usage = 
"This program tests Erasure Code decoding for all combinations of bytes lost.\n\n"
"usage: " PROG_NAME " k p l\n"
"k: number of randomly generated input bytes\n"
"p: number of parity bytes to generate\n"
"l: number of bytes loss\n"
"Example: " PROG_NAME " 4 2 2\n\n";

const char * mem_err = "Error allocating memory.";

struct queue * queue;

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
    printf("Adding to queue: ");
    for (int i = 0; i < len; i++)
        printf("%d ", array[i]);
    printf("\n");
    queue_put(queue, array);
}

void * decode_one(void * arg) {
    int * loss_idx = 0;
    struct thread_data * thread_data = arg;

    printf("String thread...\n");

    loss_idx = malloc(sizeof(*loss_idx) * thread_data->k);
    if (!loss_idx) {
        printf("%s\n", mem_err);
        return 0;
    }

    while (1) {
        queue_get(queue, loss_idx);
        printf("Received: ");
        for (int i = 0; i < thread_data->p; i++) {
            printf("%d ", loss_idx[i]);
        }
        printf("\n");

        pthread_mutex_lock(&res.lock);
        res.passed++;
        pthread_mutex_unlock(&res.lock);
    } // while (1)

    free(loss_idx);

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
    uint32_t l = 0;
    uint8_t * ec_code = 0;
    uint8_t * input = 0;
    uint8_t * result = 0;
    uint64_t counter = 0;
    int * indices = 0;
    int * loss_idx = 0;
    int i = 0;
    int num_threads = 0;
    int rc = 0;
    struct thread_data thread_data;

    if (argc != 4) {
        printf("Requires 3 parameters.\n\n");
        printf("%s\n\n", usage);
        exit(1);
    }

    k = atoi(argv[1]);
    p = atoi(argv[2]);
    l = atoi(argv[3]);

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

    input = malloc(sizeof(*input) * k);
    if (!input) {
        printf("%s\n", mem_err);
        rc = -1;
        goto err;
    }

    indices = malloc(sizeof(*indices) * k);
    if (!indices) {
        printf("%s\n", mem_err);
        rc = -1;
        goto err;
    }

    result = malloc(sizeof(*result) * k);
    if (!result) {
        printf("%s\n", mem_err);
        rc = -1;
        goto err;
    }

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
    num_threads = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    printf("Starting %d threads\n", num_threads);

    thread_data.k = k;
    thread_data.p = p;

    pthread_t * threads = malloc(sizeof(*threads) * num_threads);
    for (i = 0; i < num_threads; i++) {
        rc = pthread_create(&threads[i], NULL, decode_one, &thread_data);
        if (rc) {
            printf("Error creating thread. (error=%d)\n", rc);
            goto err;
        }
    }

    // generate combinations and signal to threads
    printf("generate combinations, %d choose %d...\n", k, p);
    loss_idx = malloc(sizeof(*loss_idx) * p);
    if (!loss_idx) {
        printf("%s\n", mem_err);
        rc = -1;
        goto err;
    }

    combination(k, p, loss_idx, 0, 0, &counter, &signal_thread, NULL);

    printf("%lu combinations found.\n", counter);

    // when we got all results, exit
    while (1) {
        pthread_mutex_lock(&res.lock);
        if (res.passed + res.failed >= counter) {
            printf("Results: %lu of %lu passed.\n",
                   res.passed, res.passed + res.failed);
            break;
        }
        pthread_mutex_unlock(&res.lock);
        printf("waiting...\n");
        sleep(2);
    }

    // Just exist withing joining threads.
#if 0
    // join threads
    printf("join threads\n");
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
#endif

err:
    // clean up
    free(loss_idx);
    queue_cleanup(queue);
    free(result);
    free(indices);
    free(input);
    free(ec_code);
    ec_cleanup();

    return rc;
}

#if 0

    // Pick k bytes out of the erasure code
    for (i = 0; i < k; i++) {
        int idx = 0;
        int duplicate = 1; 
        
        while (duplicate) {
            idx = rand() % (k + p);
            duplicate = 0;
            for (int j = 0; j < i; j++) {
                if (idx == indices[j]) {
                    duplicate = 1;
                    break;
                }
            }
        }

        indices[i] = idx;
        input[i] = ec_code[idx];
    }

    printf("Indices: ");
    print_array(indices, k);

    ec_decode(input, indices, result);

    printf("Result: ");
    print_array8(result, k);
    
    for (i = 0; i < k; i++)
        if (ec_code[i] != result[i]) {
            printf("ERROR: Incorrect decoded bytes.\n");
            rc = -2;
            goto err;
        }
    
    printf("Decode successful!\n");


}
#endif
