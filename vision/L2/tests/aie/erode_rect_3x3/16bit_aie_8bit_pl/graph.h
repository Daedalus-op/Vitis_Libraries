/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include "config.h"
#include "kernels.h"
#include <adf.h>

static constexpr int ELEM_WITH_METADATA = TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / 2);

using namespace adf;

class erodeGraph : public adf::graph {
   private:
    kernel k1;

   public:
    input_plio in1;
    output_plio out;

    erodeGraph() {
        // create kernels
        k1 = kernel::create(erode_rect_3x3);

        in1 = input_plio::create("DataIn1", adf::plio_128_bits, "data/input.txt");
        out = output_plio::create("DataOut1", adf::plio_128_bits, "data/output.txt");

        // create nets to connect kernels and IO ports
        connect<>(in1.out[0], k1.in[0]);
        connect<>(k1.out[0], out.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};

        // specify kernel sources
        source(k1) = "xf_erode_rect_3x3.cc";

        // specify kernel run times
        runtime<ratio>(k1) = 0.5;
    }
};
#endif
