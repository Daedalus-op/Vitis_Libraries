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

#include "kernels.h"
#include "imgproc/xf_blacklevel_aie.hpp"

void blackLevelCorrection(adf::input_buffer<int16>& input,
                          adf::output_buffer<int16>& output,
                          const int16_t& black_level,
                          const int32_t& mul_fact) {
    xf::cv::aie::blackLevelCorrection_api(input, output, black_level, mul_fact);
};
