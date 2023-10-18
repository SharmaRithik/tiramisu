#ifndef MATRIX_MUL_H
#define MATRIX_MUL_H

#include <tiramisu/utils.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Multiplies two matrices.
 *
 * @param b1 - Buffer for the first matrix.
 * @param b2 - Buffer for the second matrix.
 * @param b3 - Buffer to store the result of the multiplication.
 * @return 0 on success.
 */
int matmul(halide_buffer_t *b1, halide_buffer_t *b2, halide_buffer_t *b3);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // MATRIX_MUL_H

