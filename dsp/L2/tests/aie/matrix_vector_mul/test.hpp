/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

/*
This file holds the declaraion of the test harness graph class for the
matrix_vector_mul graph class.
*/
#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH matrix_vector_mul_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

class test_graph : public graph {
   private:
   public:
    std::array<input_plio, CASC_LEN> inA;
    std::array<input_plio, CASC_LEN> inB;
    std::array<output_plio, 1> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples A   = %d \n", INPUT_SAMPLES_A);
        printf("Input window A [B]= %lu \n", INPUT_SAMPLES_A * sizeof(DATA_A));
        printf("Input samples B   = %d \n", INPUT_SAMPLES_B);
        printf("Input window B [B]= %lu \n", INPUT_SAMPLES_B * sizeof(DATA_B));
        printf("Output samples  = %d \n", OUTPUT_SAMPLES);
        printf("Shift           = %d \n", SHIFT);
        printf("ROUND_MODE      = %d \n", ROUND_MODE);
        printf("Data type       = ");
        printf(QUOTE(DATA_A) QUOTE(DATA_B));
        printf("\n");
        printf("DIM_A           = %d \n", DIM_A);
        printf("DIM_B      = %d \n", DIM_B);
        printf("\n");
        printf("\n");
        printf("========================\n");

        namespace dsplib = xf::dsp::aie;
        dsplib::matrix_vector_mul::UUT_GRAPH<DATA_A, DATA_B, DIM_A, DIM_B, SHIFT, ROUND_MODE, NUM_FRAMES, CASC_LEN>
            matrix_vector_mulGraph;
#ifdef USING_UUT
        for (int i = 0; i < CASC_LEN; i++) {
            std::string filenameInMatrix = QUOTE(INPUT_FILE_A);
            std::string filenameInVector = QUOTE(INPUT_FILE_B);
            // filenameInMatrix.insert(filenameInMatrix.length()-4, ("_"+std::to_string(i)+"_0"));
            // filenameInVector.insert(filenameInVector.length()-4, ("_"+std::to_string(i)+"_0"));

            inA[i] = input_plio::create("PLIO_in_A" + std::to_string(i), adf::plio_32_bits, filenameInMatrix);
            inB[i] = input_plio::create("PLIO_in_B" + std::to_string(i), adf::plio_32_bits, filenameInVector);

            connect<>(inA[i].out[0], matrix_vector_mulGraph.inA[i]);
            connect<>(inB[i].out[0], matrix_vector_mulGraph.inB[i]);
        }

#else // using ref
        std::string filenameInMatrix = QUOTE(INPUT_FILE_A);
        std::string filenameInVector = QUOTE(INPUT_FILE_B);
        // Make connections
        inA[0] = input_plio::create("PLIO_in_A" + std::to_string(0), adf::plio_32_bits, filenameInMatrix);
        inB[0] = input_plio::create("PLIO_in_B" + std::to_string(0), adf::plio_32_bits, filenameInVector);
        connect<>(inA[0].out[0], matrix_vector_mulGraph.inA[0]);
        connect<>(inB[0].out[0], matrix_vector_mulGraph.inB[0]);
#endif

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        // filenameOut.insert(filenameOut.length()-4, ("_0_0"));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_32_bits, filenameOut);
        connect<>(matrix_vector_mulGraph.out[0], out[0].in[0]);
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
