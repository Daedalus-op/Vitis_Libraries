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
#include "kernels.h"
#include "config.h"

using namespace adf;

class blacklevelGraph : public adf::graph {
   private:
    kernel k1;

   public:
    port<input> blacklevel;
    port<input> mulfact;

    input_gmio in1;
    output_gmio out1;

    blacklevelGraph() {
        // create kernels
        k1 = kernel::create(blackLevelCorrection);

        in1 = input_gmio::create("gmioIn1", 256, 1000);
        out1 = output_gmio::create("gmioOut1", 256, 1000);

        // create nets to connect kernels and IO ports
        adf::connect<>(in1.out[0], k1.in[0]);
        adf::connect<>(k1.out[0], out1.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};

        connect<parameter>(blacklevel, async(k1.in[1]));
        connect<parameter>(mulfact, async(k1.in[2]));

        // specify kernel sources
        source(k1) = "xf_blacklevel.cc";

        // specify kernel run times
        runtime<ratio>(k1) = 0.5;
    }
};
#endif
