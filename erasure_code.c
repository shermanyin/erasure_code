#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gf_base2.h"

struct ec_config {
    uint32_t k; // number of input bytes
    uint32_t p; // number of parity bytes
    uint32_t n; // number of output bytes
    struct gf_matrix * matrix; // encoding matrix
};

struct ec_config ec;

/*
 * Assume matrix is r by c, where r > c.
 */
void
cauchy_matrix_gen(struct gf_matrix * m) {
    // top rows form the identity matrix
    gf_matrix_identity_set(m);

    // calculate bottom rows
    for (int r = m->cols; r < m->rows; r++)
        for (int c = 0; c < m->cols; c++)
            m->v[r * m->cols + c] = gf_mult_inv(gf_add(r, c));
}

void
rs_matrix_gen(struct gf_matrix * m) {
    // top rows form the identity matrix
    gf_matrix_identity_set(m);

    // calculate bottom rows
    for (int r = m->cols; r < m->rows; r++)
        for (int c = 0; c < m->cols; c++)
            m->v[r * m->cols + c] = gf_pow(2, (r - m->cols) * c);
}

int
vandermonde_matrix_gen(struct gf_matrix * m) {
    uint8_t mult_inv = 0;

    // create the vandermonde matrix
    for (int r = 0; r < m->rows; r++)
        for (int c = 0; c < m->cols; c++)
            m->v[r * m->cols + c] = gf_pow(r, c);

    printf("Vandermonde matrix b4 transformation:\n");
    gf_matrix_print(m);

    // transform matrix to get identity matrix for the top k rows
    // note: use column transformations.  row transformation result
    // in a matrix that is sometimes non-invertible.
    for (int col = 0; col < m->cols; col++) {
        // index of the pivot
        int pivot = col * m->cols + col;

        // if the pivot is zero, find another col to swap
        if (!m->v[pivot]) {
            for (int col2 = col + 1; col2 < m->cols; col2++) {
                if (m->v[col * m->cols + col2]) {
                    gf_matrix_swap_cols(m, col, col2);
                    break;
                }
            }
        }

        switch (m->v[pivot]) {
            case 0:
                printf("Cannot properly transform Vandermonde matrix:\n");
                gf_matrix_print(m);
                return -1;

            case 1:
                // no-op
                break;

            default:
                // scale col i so pivot is 1
                mult_inv = gf_mult_inv(m->v[pivot]);
                for (int row = 0; row < m->rows; row++) {
                    int idx = row * m->cols + col;
                    m->v[idx] = gf_mult(mult_inv, m->v[idx]);
                }
        } // switch pivot value

        // zero out the ith row in other cols 
        for (int col2 = 0; col2 < m->cols; col2++) {
            uint8_t scale = 0;

            // skip the current col
            if (col2 == col)
                continue;

            scale = m->v[col * m->cols + col2];
            for (int row = 0; row < m->rows; row++) {
                m->v[row * m->cols + col2]
                    = gf_add(m->v[row * m->cols + col2], gf_mult(scale, m->v[row * m->cols + col]));
            }
        }
    } // for the top square matrix

    return 0;
}

void
ec_cleanup() {
    gf_cleanup();
    gf_matrix_delete(ec.matrix);
}

int
ec_init(const uint32_t k, const uint32_t p) {
    memset(&ec, 0, sizeof(ec));
    ec.k = k;
    ec.p = p;
    ec.n = k + p;

    ec.matrix = gf_matrix_create(ec.n, ec.k);
    if (!ec.matrix) {
        printf("Failed to create matrix.\n");
        ec_cleanup();
        return -1;
    }

    // Need to initialize GF before doing any math
    int rc = gf_init(8, 283);
    if (rc) {
        printf("Error initializing Galois Field.\n");
        ec_cleanup();
        return -1;
    }

    //cauchy_matrix_gen(ec.matrix);
    //rs_matrix_gen(ec.matrix);
    rc = vandermonde_matrix_gen(ec.matrix);
    if (rc) {
        printf("Error generating encoding matrix.\n");
        return rc;
    }

    printf("Encoding matrix:\n");
    gf_matrix_print(ec.matrix);

    printf("Erasure Code module initialized.\n");

    return 0;
}

int
ec_encode(uint8_t * input, uint8_t * parity) {
    int rc = 0;

    // The bottom part of the encoding matrix is used for encoding.  
    struct gf_matrix encoding_m = {
        .rows = ec.p,
        .cols = ec.k,
        .v = &(ec.matrix->v[ec.k * ec.k]),
    };

    // Convert the input into a matrix
    struct gf_matrix input_m = {
        .rows = ec.k,
        .cols = 1,
        .v = input,
    };

    // Convert the parity output into a matrix
    struct gf_matrix parity_m = {
        .rows = ec.p,
        .cols = 1,
        .v = parity,
    };
    
    return gf_matrix_mult(&encoding_m, &input_m, &parity_m);
}

int
ec_decode(uint8_t * input, int * indices, uint8_t * result) {
    int rc = 0;
    int row = 0;

    struct gf_matrix input_m = {
        .rows = ec.k,
        .cols = 1,
        .v = input,
    };

    struct gf_matrix result_m = {
        .rows = ec.k,
        .cols = 1,
        .v = result,
    };

    struct gf_matrix * decode_m = gf_matrix_create(ec.k, ec.k);
    struct gf_matrix * decode_inv_m = gf_matrix_create(ec.k, ec.k);

    // copy the rows corresponding to the indices from ec.matrix to decode_m
    for (int i = 0; i < ec.k; i++) {
        row = indices[i];
        for (int j = 0; j < ec.k; j++)
            decode_m->v[i * ec.k + j] = ec.matrix->v[row * ec.k + j];
    }

    rc = gf_matrix_inv(decode_m, decode_inv_m); 
    if (rc) {
        printf("Error decoding - cannot find inverse of encoding matrix.\n");
        printf("Input was:\n");
        for (int i = 0; i < ec.k; i++) {
            printf("%02x ", input[i]);
        }
        printf("\n");
        printf("Indices was:\n");
        for (int i = 0; i < ec.k; i++) {
            printf("%d ", indices[i]);
        }
        printf("\n");
        goto decode_err;
    }

    gf_matrix_mult(decode_inv_m, &input_m, &result_m);

decode_err:
    gf_matrix_delete(decode_inv_m);
    gf_matrix_delete(decode_m);

    return rc;
}
