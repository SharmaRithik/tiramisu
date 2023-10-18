#include <tiramisu/tiramisu.h>
#include <fstream>
#include <iostream>
#include <vector>

#define SIZE0 1000

using namespace tiramisu;

int main(int argc, char **argv)
{
    // Reading matrices from files
    std::vector<std::vector<float>> matrixA(SIZE0, std::vector<float>(SIZE0));
    std::vector<std::vector<float>> matrixB(SIZE0, std::vector<float>(SIZE0));

    std::ifstream inputA("matrix_sparse.mtx");
    std::ifstream inputB("matrix_dense.mtx");

    for (int i = 0; i < SIZE0; i++) {
        for (int j = 0; j < SIZE0; j++) {
            inputA >> matrixA[i][j];
            inputB >> matrixB[i][j];
        }
    }
    inputA.close();
    inputB.close();

    tiramisu::init("matmul");

    constant p0("N", expr((int32_t) SIZE0));
    var i("i", 0, p0), j("j", 0, p0), k("k", 0, p0);

    input A("A", {"i", "j"}, {SIZE0, SIZE0}, p_float32);
    input B("B", {"i", "j"}, {SIZE0, SIZE0}, p_float32);

    computation C_init("C_init", {i,j}, expr((float) 0));
    computation C("C", {i,j,k}, p_float32);
    C.set_expression(C(i, j, k - 1) + A(i, k) * B(k, j));

    buffer b_A("b_A", {expr(SIZE0), expr(SIZE0)}, p_float32, a_input);
    buffer b_B("b_B", {expr(SIZE0), expr(SIZE0)}, p_float32, a_input);
    buffer b_C("b_C", {expr(SIZE0), expr(SIZE0)}, p_float32, a_output);

    A.store_in(&b_A);
    B.store_in(&b_B);
    C_init.store_in(&b_C, {i, j});
    C.store_in(&b_C, {i, j});

    for (int m = 0; m < SIZE0; m++) {
        for (int n = 0; n < SIZE0; n++) {
            b_A(n, m) = matrixA[m][n];
            b_B(n, m) = matrixB[m][n];
        }
    }

    tiramisu::codegen({&b_A, &b_B, &b_C}, "build/matrix_mul.o");

    return 0;
}

