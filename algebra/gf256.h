#ifndef GF256_H
#define GF256_H

#include <stdint.h>

uint8_t gf256_add(uint8_t a, uint8_t b);
#define gf256_sub(a, b)	gf256_add((a), (b))

int gf256_log(uint8_t val);
uint8_t gf256_exp(int e);

uint8_t gf256_mul(uint8_t a, uint8_t b);
uint8_t gf256_inv(uint8_t v);

#endif /* GF256_H */

