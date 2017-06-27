#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct gf_base2 {
    uint32_t m;     // degree of GF (limited to 8 due to choice of uint8_t)
    uint32_t g;     // coefficients of irreducible polynomial g(x)
    uint32_t order; // number of elements in this Galois Field

    uint8_t * mult_tbl;     //multiplcation table
    uint8_t * mult_inv_tbl; //mlutiplicative inverse table
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
    uint32_t prod = 0;
    uint32_t g_msb = 1 << gf.m;

    // Multiply
    while (x) {
        if (x & 1)
            prod ^= y;
        x >>= 1;
        y <<= 1;
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

    memset(&gf, 0, sizeof(gf));
    
    gf.m = m;
    gf.g = g;
    gf.order = 1 << m;

    /* table is 2^m x 2^m entries */
    gf.mult_tbl = malloc(sizeof(uint8_t) * gf.order * gf.order);
    if (!gf.mult_tbl) {
        printf("%s", mem_err);
        gf_cleanup();
        return -1;
    }

    for (int row = 0; row < gf.order; row++)
        for (int col = 0; col < gf.order; col++)
            gf.mult_tbl[row * gf.order + col] = gf_long_mult(row, col);

    /* table is 2^m x 1 entries */
    gf.mult_inv_tbl = malloc(sizeof(uint8_t) * gf.order);
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

