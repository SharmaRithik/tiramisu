#include "Halide.h"
#include "matrix_mul.h"
#include <tiramisu/utils.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>

#define NN 1000

int main(int, char **)
{
    Halide::Buffer<float> A_buf(NN, NN);
    Halide::Buffer<float> B_buf(NN, NN);

    // Reading matrices from files
    std::ifstream inputA("matrix_sparse.mtx");
    std::ifstream inputB("matrix_dense.mtx");

    for (int i = 0; i < NN; i++) {
        for (int j = 0; j < NN; j++) {
            inputA >> A_buf(j, i);
            inputB >> B_buf(j, i);
        }
    }
    inputA.close();
    inputB.close();

    Halide::Buffer<float> C1_buf(NN, NN);

    matmul(A_buf.raw_buffer(), B_buf.raw_buffer(), C1_buf.raw_buffer());

    // Reference matrix multiplication
    Halide::Buffer<float> C2_buf(NN, NN);
    init_buffer(C2_buf, (float)0);

    for (int i = 0; i < NN; i++) {
        for (int j = 0; j < NN; j++) {
            for (int k = 0; k < NN; k++) {
                C2_buf(j, i) += A_buf(k, i) * B_buf(j, k);
            }
        }
    }

    compare_buffers("matmul", C1_buf, C2_buf);

    return 0;
}

