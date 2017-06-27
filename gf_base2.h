#ifndef GF_BASE2_H
#define GF_BASE2_H

#include <stdint.h>

int gf_init(const uint32_t m, const uint32_t g);

void gf_cleanup();

uint8_t gf_add(uint8_t x, uint8_t y);

uint8_t gf_add_inv(uint8_t x);

uint8_t gf_mult(uint8_t x, uint8_t y);

uint8_t gf_mult_inv(uint8_t x);

uint8_t gf_pow(uint8_t x, uint8_t y);

void gf_print_mult_tbl();

void gf_print_mult_inv_tbl();

#endif
