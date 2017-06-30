#ifndef GF_BASE2_H
#define GF_BASE2_H

#include <stdint.h>

// structure representing a Matrix
struct gf_matrix {
    int rows;       // number of rows
    int cols;       // number of columns
    uint8_t * v;    // values as a one-dimensional array
};

int gf_init(const uint32_t m, const uint32_t g);

void gf_cleanup();

uint8_t gf_add(uint8_t x, uint8_t y);

uint8_t gf_add_inv(uint8_t x);

uint8_t gf_mult(uint8_t x, uint8_t y);

uint8_t gf_mult_inv(uint8_t x);

uint8_t gf_pow(uint8_t x, uint8_t y);

void gf_print_mult_tbl();

void gf_print_mult_inv_tbl();

struct gf_matrix * gf_matrix_create(int rows, int cols);

struct gf_matrix * gf_matrix_create_from(struct gf_matrix * x);

void gf_matrix_delete(struct gf_matrix * x);

void gf_matrix_print(struct gf_matrix * x);

/*
 * Generates an identity matrix in the given matrix. If the matrix is not a
 * square, generates the largest identity matrix within it.
 */
void gf_matrix_identity_set(struct gf_matrix * x);

/*
 * Multiply the specified row of matrix x by the specified column of matrix y
 */
uint8_t gf_matrix_dot_prod(struct gf_matrix * x,
                           struct gf_matrix * y,
                           int row,
                           int col);

/*
 * Multiply matrix x by matrix y.  Number of columns in X must equal to number
 * of rows in Y.  x, y, result are expected to be pre-allocated matrices.
 *
 * x (IN):     left operand, an a x b matrix 
 * y (IN):     right operand, a b x c matrix
 * prod (OUT): product, an a x c matrix
 */
int gf_matrix_mult(struct gf_matrix * x,
                   struct gf_matrix * y,
                   struct gf_matrix * prod);

int gf_matrix_inv(struct gf_matrix * x, struct gf_matrix * inv);
#endif
