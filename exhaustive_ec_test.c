#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "erasure_code.h"
#include "queue.h"

#define PROG_NAME "exhaustive_ec_test"

const char * usage = 
"This program tests Erasure Code decoding for all combinations of bytes lost.\n\n"
"usage: " PROG_NAME " k p l\n"
"k: number of randomly generated input bytes\n"
"p: number of parity bytes to generate\n"
"l: number of bytes loss\n"
"Example: " PROG_NAME " 4 2 2\n\n";

const char * mem_err = "Error allocating memory.";

pthread_mutex_t queue_lock;

void print_array(int * a, size_t len) {
    for (int i = 0; i < len; i++)
        printf("%2x, ", (int)a[i]);
    printf("\n");
}

void print_array8(uint8_t * a, size_t len) {
    for (int i = 0; i < len; i++)
        printf("%2x, ", (int)a[i]);
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
                 void (*fcn)(int *, int, void *),
                 void * arg) {
    if (pos == r) {
        (*fcn)(comb, r, arg);
        return;
    }

    for (int i = start; i <= n - r + pos; i++) {
        comb[pos] = i;
        combination(n, r, comb, pos + 1, i + 1, fcn, arg);
    }
}

void print_res(int * array, int len, void * arg) {
    for (int i = 0; i < len; i++)
        printf("%d ", array[i]);
    printf("\n");
}

void * decode_one(void * idx) {
    int * loss_idx = idx;

    pthread_mutex_lock(&queue_lock);
    printf("This is thread %d\n", (*loss_idx)++); 
    pthread_mutex_unlock(&queue_lock);
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
    int * indices = 0;
    int * loss_idx = 0;
    int i = 0;
    int num_threads = 0;
    int rc = 0;

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

    printf("Original data: ");
    print_array8(ec_code, k);

    printf("Erasure Code: ");
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

    // init queue

    // init mutex
    rc = pthread_mutex_init(&queue_lock, NULL);
    if (rc) {
        printf("Error initializing mutex. Error=%d.\n", rc);
        goto err;
    }
    
    // init semaphore
    
    // start threads
    num_threads = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    printf("start %d threads\n", num_threads);

    int a = 0;
    pthread_t * threads = malloc(sizeof(*threads) * num_threads);
    for (i = 0; i < num_threads; i++) {
        rc = pthread_create(&threads[i], NULL, decode_one, &a);
        if (rc) {
            printf("Error creating thread. (error=%d)\n", rc);
            goto err;
        }
    }

    // generate combinations and signal to threads
    printf("generate combinations\n");
    loss_idx = malloc(sizeof(*loss_idx) * p);
    if (!loss_idx) {
        printf("%s\n", mem_err);
        rc = -1;
        goto err;
    }

    combination(k, p, loss_idx, 0, 0, &print_res, NULL);

    // join threads
    printf("join threads\n");
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

err:
    // clean up
    free(loss_idx);
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
