#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "erasure_code.h"

#define PROG_NAME "encode_decode"

const char * usage = 
"usage: " PROG_NAME " k p\n"
"Example: " PROG_NAME " 4 2\n\n";

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

int main(int argc, char* argv[]) {
    uint32_t k = 0;
    uint32_t p = 0;
    int rc = 0;
    uint8_t * ec_code = 0;
    uint8_t * parity = 0;
    uint8_t * input = 0;
    int * indices = 0;
    uint8_t * result = 0;
    size_t sz8 = 0;
    int i = 0;

    if (argc != 3) {
        printf("Requires 2 parameters.\n\n");
        printf("%s\n\n", usage);
        exit(1);
    }

    srand(time(NULL));

    k = atoi(argv[1]);
    p = atoi(argv[2]);
    
    rc = ec_init(k, p);

    if (rc) {
        printf("Error initializing Erasure Code.\n");
        exit(1);
    }

    sz8 = sizeof(uint8_t);
    
    ec_code = malloc(sz8 * (k + p));
    if (!ec_code) {
        printf("Error allocating memory.\n");
        rc = -1;
        goto err;
    }

    input = malloc(sz8 * k);
    if (!input) {
        printf("Error allocating memory.\n");
        rc = -1;
        goto err;
    }

    indices = malloc(sizeof(int) * k);
    if (!indices) {
        printf("Error allocating memory.\n");
        rc = -1;
        goto err;
    }

    result = malloc(sz8 * k);
    if (!result) {
        printf("Error allocating memory.\n");
        rc = -1;
        goto err;
    }

    parity = ec_code;

    // Random source data
    for (i = 0; i < k; i++, parity++)
        ec_code[i] = (uint8_t) (rand() % (UINT8_MAX + 1));

    printf("Original data: ");
    print_array8(ec_code, k);

    rc = ec_encode(ec_code, parity);

    printf("Erasure Code: ");
    print_array8(ec_code, k + p);

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


err:
    free(result);
    free(indices);
    free(input);
    free(ec_code);
    ec_cleanup();

    return rc;
}
