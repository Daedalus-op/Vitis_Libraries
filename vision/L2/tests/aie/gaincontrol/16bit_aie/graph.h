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

#include <adf.h>
#include <common/xf_aie_const.hpp>
#include "kernels.h"

static constexpr int TILE_ELEMENTS = 4096;
static constexpr int TILE_WINDOW_SIZE = TILE_ELEMENTS * sizeof(int16_t) + xf::cv::aie::METADATA_SIZE;
static constexpr int ELEM_WITH_METADATA = TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(int16_t));
static constexpr int XF_BAYER_RG = 0;
static constexpr int XF_BAYER_GR = 1;
static constexpr int XF_BAYER_BG = 2;
static constexpr int XF_BAYER_GB = 3;

using namespace adf;

/*
 * Cardano dataflow graph to compute weighted moving average of
 * the last 8 samples in a stream of numbers
 */

class gaincontrolGraph : public adf::graph {
   private:
    kernel k1;

   public:
    input_plio in1;
    port<input> rgain;
    port<input> bgain;
    output_plio out;

    gaincontrolGraph() {
        // create kernels
        k1 = kernel::create(gaincontrol<XF_BAYER_RG>);

        in1 = input_plio::create("DataIn1", adf::plio_64_bits, "data/input.txt");
        out = output_plio::create("DataOut1", adf::plio_64_bits, "data/output.txt");

        // create nets to connect kernels and IO ports
        adf::connect<>(in1.out[0], k1.in[0]);
        adf::connect<>(k1.out[0], out.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};

        connect<parameter>(rgain, async(k1.in[1]));
        connect<parameter>(bgain, async(k1.in[2]));

        // specify kernel sources
        source(k1) = "xf_gaincontrol.cc";

        // specify kernel run times
        runtime<ratio>(k1) = 0.5;
    }
};

#endif
