/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#include <adf.h>
#include "conv_corr_graph.hpp"

using namespace adf;
namespace corr_example {

#define DATATYPE_F_CORR int16
#define DATATYPE_G_CORR int16
#define DATATYPE_OUT_CORR int32
#define FUNCT_TYPE_CORR 0
#define COMPUTE_MODE_CORR 0
#define F_LEN_CORR 128
#define G_LEN_CORR 32
#define SHIFT_CORR 7
#define API_CORR 0

class test_corr : public adf::graph {
   public:
    port<input> inF;
    port<input> inG;
    port<output> out;
    xf::dsp::aie::conv_corr::conv_corr_graph<DATATYPE_F_CORR,
                                             DATATYPE_G_CORR,
                                             DATATYPE_OUT_CORR,
                                             FUNCT_TYPE_CORR,
                                             COMPUTE_MODE_CORR,
                                             F_LEN_CORR,
                                             G_LEN_CORR,
                                             SHIFT_CORR,
                                             API_CORR>
        corr;
    test_corr() {
        connect<>(inF, corr.inWindowF);
        connect<>(inG, corr.inWindowG);
        connect<>(corr.outWindow, out);
    };
};
};