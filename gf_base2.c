#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gf_base2.h"

struct gf_base2 {
    uint32_t m;     // degree of GF (limited to 8 due to choice of uint8_t)
    uint32_t g;     // coefficients of irreducible polynomial g(x)
    uint32_t order; // number of elements in this Galois Field

    uint8_t * mult_tbl;     //multiplcation table
    uint8_t * mult_inv_tbl; //multiplicative inverse table
};

/* static struct for functions in this file only */
struct gf_base2 gf;

void
gf_cleanup() {
    if (gf.mult_tbl) 
        free(gf.mult_tbl);

    if (gf.mult_inv_tbl)
        free(gf.mult_inv_tbl);
}

uint8_t
gf_long_mult(uint8_t x, uint8_t y) {
    uint32_t yy = y; // may need to shift y to left
    uint32_t prod = 0;
    uint32_t g_msb = 1 << gf.m;

    // Multiply
    while (x) {
        if (x & 1)
            prod ^= yy;
        x >>= 1;
        yy <<= 1;
    }

    // Reduce by g(x)
    for (int i = gf.m - 2; i >= 0; i--)
        if (prod & (g_msb << i))
            prod ^= (gf.g << i);

    return (uint8_t) prod;
}

int
gf_init(const uint32_t m, const uint32_t g) {
    const char * mem_err =
        "Error allocating memory. GF initialization failed.\n";

    /* Supports only up to degree 8 due to choice of uint8_t */
    if (m > 8) {
        printf(
            "Unsupported value for degree %d. "
            "Currently only support GF base 2 fields of up to 8th degree. "
            "GF initialzation failed.\n",
            m
        );
        return -1;
    }

    /* Check degree of g(x) */
    if ((g >> m) != 1) {
        printf(
            "Incorrect value for g(x) = 0x%x. Highest degree of g(x) must be "
            "equal to m (%d). GF initialization failed.\n",
            g, m
        );
        return -1;
    }

    printf("Initializing GF(2^%d)...\n", m);

    memset(&gf, 0, sizeof(gf));
    
    gf.m = m;
    gf.g = g;
    gf.order = 1 << m;

    /* table is 2^m x 2^m entries */
    gf.mult_tbl = malloc(sizeof(*gf.mult_tbl) * gf.order * gf.order);
    if (!gf.mult_tbl) {
        printf("%s", mem_err);
        gf_cleanup();
        return -1;
    }

    for (int row = 0; row < gf.order; row++)
        for (int col = 0; col < gf.order; col++)
            gf.mult_tbl[row * gf.order + col] = gf_long_mult(row, col);

    /* table is 2^m x 1 entries */
    gf.mult_inv_tbl = malloc(sizeof(*gf.mult_inv_tbl) * gf.order);
    if (!gf.mult_inv_tbl) {
        printf("%s", mem_err);
        gf_cleanup();
        return -1;
    }

    /* Loop starts at 1; mult. inverse for 0 is undefined. */
    for (int i = 1; i < gf.order; i++) {
        for (int j = 0; j < gf.order; j++) {
            if (gf.mult_tbl[i * gf.order + j] == 1) {
                gf.mult_inv_tbl[i] = j;
                break;
            }
        }
    }

    printf("GF(2^%d) initialization completed.\n\n", m);
    
    return 0;
}

uint8_t
gf_add(uint8_t x, uint8_t y) {
    return x ^ y;
}

uint8_t
gf_add_inv(uint8_t x) {
    return x;
}

uint8_t
gf_mult(uint8_t x, uint8_t y) {
    return gf.mult_tbl[x * gf.order + y];
}

uint8_t
gf_mult_inv(uint8_t x) {
    return gf.mult_inv_tbl[x];
}

uint8_t
gf_pow(uint8_t x, uint8_t y) {
    uint8_t res = 1;
    for (int i = 0; i < y; i++) {
        res = gf_mult(res, x);
    }
    return res;
}

void
gf_matrix_print(struct gf_matrix * x) {
    for (int i = 0; i < x->rows; i++) {
        for (int j = 0; j < x->cols; j++)
            printf("%02x ", x->v[i * x->cols + j]);
        printf("\n");
    }
}

void
gf_print_mult_tbl() {
    int i = 0;
    int j = 0;

    printf(
        "Multiplication table for GF(2^%d) with g(x) = 0x%x\n\n",
        gf.m, gf.g
    );

    /* Leave space for row headers */
    printf("     ");

    /* Print column headers */
    for (i = 0; i < gf.order; i++)
        printf("%02x ", i);
    printf("\n");

    printf("   + ");
    for (i = 0; i < gf.order; i++)
        printf("---");
    printf("\n");

    /* Print each row */
    for (i = 0; i < gf.order; i++) {
        printf("%02x | ", i);
        for (j = 0; j < gf.order; j++) {
            printf("%02x ", gf.mult_tbl[i * gf.order + j]);
        }
        printf("\n");
    }

    printf("\n");
}

void
gf_print_mult_inv_tbl() {
    int i = 0;
    int j = 0;

    printf(
        "Multiplicative inverse  table for GF(2^%d) with g(x) = 0x%x\n\n",
        gf.m, gf.g
    );

    for (i = 1; i < gf.order; i++)
        printf("%02x : %02x\n", i, gf.mult_inv_tbl[i]);
    
    printf("\n");
}

struct gf_matrix *
gf_matrix_create(int rows, int cols) {
    const char * mem_err =
        "Error allocating memory. Matrix initialization failed.";

    int m_size = sizeof(uint8_t) * rows * cols;
    
    struct gf_matrix * m = malloc(sizeof(*m));
    if (!m) {
        printf("%s\n", mem_err);
        return NULL;
    }

    memset(m, 0, sizeof(*m));

    m->rows = rows;
    m->cols = cols;

    m->v = malloc(m_size);
    if (!m->v) {
        printf("%s\n", mem_err);
        return NULL;
    }

    memset(m->v, 0, m_size);

    return m;
}

struct gf_matrix *
gf_matrix_create_from(struct gf_matrix * x) {
    struct gf_matrix * m = gf_matrix_create(x->rows, x->cols);

    if (!m)
        return NULL;

    for (int i = 0; i < x->rows * x-> cols; i++)
        m->v[i] = x->v[i];

    return m;
}

void
gf_matrix_delete(struct gf_matrix * x) {
    if (x) {
        if (x->v)
            free(x->v);
        free(x);
    }
}

void
gf_matrix_identity_set(struct gf_matrix * x) {
    int n = (x->rows < x->cols) ? x->rows : x->cols;

    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++)
            x->v[r * x->cols + c] = (r == c) ? 1 : 0;
}

uint8_t
gf_matrix_dot_prod(struct gf_matrix * x,
                   struct gf_matrix * y,
                   int row,
                   int col) {
    uint8_t res = 0;
    uint8_t prod = 0;

    for (int i = 0; i < x->cols; i++) {
        prod = gf_mult(x->v[row * x->cols + i], y->v[i * y->cols + col]);
        res = gf_add(res, prod);
    }

    return res;
}

int
gf_matrix_mult(struct gf_matrix * x,
               struct gf_matrix * y,
               struct gf_matrix * prod) {
    if (x->cols != y->rows) {
        printf("Invalid dimensions for matrix multiplication.\n");
        return -1;
    }

    if (x->rows != prod->rows || y->cols != prod->cols) {
        printf("Incorrect matrix dimensions to hold product..\n");
        return -1;
    }

    for (int i = 0; i < x->rows; i++)
        for (int j = 0; j < y->cols; j++)
            prod->v[i * prod->cols + j] = gf_matrix_dot_prod(x, y, i, j);

    return 0;
}

void
gf_matrix_swap_rows(struct gf_matrix * x, int row1, int row2) {
    uint8_t temp = 0;

    for (int i = 0; i < x->cols; i++) {
        temp = x->v[row1 * x->cols + i];
        x->v[row1 * x->cols + i] = x->v[row2 * x->cols + i];
        x->v[row2 * x->cols + i] = temp;
    }
}

int
gf_matrix_inv(struct gf_matrix * x, struct gf_matrix * inv) {
    int rc = 0;
    uint8_t mult_inv = 0;
    uint8_t scale = 0;
    int idx = 0;
    int col = 0;
    int row = 0;
    int row2 = 0;

    if (x->rows != x->cols) {
        printf("Non-sqaure matrices are singular.\n");
        return -1;
    }

    // make a copy of x so we don't modify the original
    struct gf_matrix * m = gf_matrix_create_from(x);

    gf_matrix_identity_set(inv);

    for (row = 0; row < m->rows; row++) {
        // index of the pivot
        int pivot = row * m->cols + row;

        // if the pivot is zero, find another row to swap
        if (!m->v[pivot]) {
            for (row2 = row + 1; row2 < m->rows; row2++) {
                if (m->v[row2 * m->cols + row]) {
                    gf_matrix_swap_rows(m, row, row2);
                    gf_matrix_swap_rows(inv, row, row2);
                    break;
                }
            }
        }
        
        switch (m->v[pivot]) {
            case 0:
                printf("Cannot find inverse for the following matrix:\n");
                gf_matrix_print(x);
                printf("Got up to here:\n");
                gf_matrix_print(m);
                rc = -1;
                goto inv_err;

            case 1:
                // no-op
                break;

            default:
                // scale row i so pivot is 1
                mult_inv = gf_mult_inv(m->v[pivot]);
                for (col = 0; col < m->cols; col++) {
                    idx = row * m->cols + col;
                    m->v[idx] = gf_mult(mult_inv, m->v[idx]);
                    inv->v[idx] = gf_mult(mult_inv, inv->v[idx]);
                }
        } // switch pivot value

        // zero out the ith column in other rows
        for (row2 = 0; row2 < m->rows; row2++) {
            // skip the current row
            if (row2 == row)
                continue;

            scale = m->v[row2 * m->cols + row];
            for (col = 0; col < m->cols; col++) {
                m->v[row2 * m->cols + col]
                    = gf_add(m->v[row2 * m->cols + col], gf_mult(scale, m->v[row * m->cols + col]));
                inv->v[row2 * inv->cols + col]
                    = gf_add(inv->v[row2 * inv->cols + col], gf_mult(scale, inv->v[row * inv->cols + col]));
            }
        }
    } // for each row in the original matrix

inv_err:
    gf_matrix_delete(m);
    return rc;
}
