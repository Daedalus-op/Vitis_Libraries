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

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <adf.h>
#include "kernels.h"
#include "config.h"

using namespace adf;

class myGraph : public adf::graph {
   private:
    kernel k1;

   public:
    input_plio inprt1;
    input_plio inprt2;

    output_plio outprt;

    myGraph() {
        k1 = kernel::create(accumulate);

        inprt1 = input_plio::create("DataIn1", adf::plio_128_bits, "data/input1.txt");
        inprt2 = input_plio::create("DataIn2", adf::plio_128_bits, "data/input2.txt");
        outprt = output_plio::create("DataOut1", adf::plio_128_bits, "data/output.txt");

        connect<>(inprt1.out[0], k1.in[0]);
        connect<>(inprt2.out[0], k1.in[1]);
        connect<>(k1.out[0], outprt.in[0]);

        adf::dimensions(k1.in[0]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.in[1]) = {ELEM_WITH_METADATA};
        adf::dimensions(k1.out[0]) = {ELEM_WITH_METADATA};
        source(k1) = "xf_accumulate.cc";
        // Initial mapping
        runtime<ratio>(k1) = 0.5;
    };
};

#endif
