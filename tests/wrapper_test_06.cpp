#include "Halide.h"
#include "wrapper_test_06.h"

#include <tiramisu/utils.h>
#include <cstdlib>
#include <iostream>

#define NN 10
#define MM 10

int main(int, char**)
{
    buffer_t reference_buf = allocate_2D_buffer(NN, MM);
    init_2D_buffer_val(&reference_buf, NN, MM, 4);

    buffer_t output_buf = allocate_2D_buffer(NN, MM);
    init_2D_buffer_val(&output_buf, NN, MM, 0);
    Halide::Buffer<uint8_t> halide_output_buf(output_buf);

    test_shift_operator(halide_output_buf.raw_buffer());

    compare_2_2D_arrays("test_shift_operator", halide_output_buf.data(), reference_buf.host, NN, MM);

   return 0;
}
