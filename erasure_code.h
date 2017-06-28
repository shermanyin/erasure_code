#ifndef ERASURE_CODE_H
#define ERASURE_CODE_H

#include <stdint.h>
/*
 * Initialize erasure code encoder/decoder
 *
 * k (IN):  number of input bytes to encode at a time
 * p (IN):  number of parity bytes to generate from the k input bytes
 */
int ec_init(const uint32_t k, const uint32_t p);

/*
 * Cleans up the erasure code encoder/decoder
 */
void ec_cleanup();

/*
 * Generate an array of parity bytes from the given input
 * 
 * input (IN):   array of k bytes to generate parity bytes for
 * parity (OUT): array of p bytes to store the generated parity bytes 
 *
 * Example:
 * Assume k = 3, p = 2, and the input array is [a b c]. After encoding, the
 * parity array will contain [d e], which is the calculated parity bytes.
 */
int ec_encode(uint8_t * input, uint8_t * parity);

/*
 * Decode the given input and recover the original data
 *
 * input (IN):   array of bytes to be decoded, which may contain both original
 *               data and parity and must be length k
 * indices (IN): array of indices from 0..(n-1) indicating the original
 *               positions of the input bytes during EC encoding, must be length
 *               k
 * result (OUT): array of k recovered original data bytes
 * 
 * Example:
 * Assume k = 3, p = 2, and the encoding result was [a b c d e], where [a b c]
 * is the original data, and [d e] are the parity bytes.  If c and d are lost,
 * then the input array to this function should be [a b e], the indicies array
 * should be [0 1 4].  After decoding, orig array will contain [a b c].
 */
int ec_decode(uint8_t * input, bool * indices, uint8_t * result);

#endif
