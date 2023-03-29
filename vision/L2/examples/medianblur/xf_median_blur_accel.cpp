/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_median_blur_accel_config.h"
extern "C" {

void medianblur_accel(ap_uint<INPUT_PTR_WIDTH>* img_in, int rows, int cols, ap_uint<OUTPUT_PTR_WIDTH>* img_out) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=img_out        offset=slave bundle=gmem1
    #pragma HLS INTERFACE s_axilite  port=rows 			       	   	  
	#pragma HLS INTERFACE s_axilite  port=cols 			              
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> imgInput(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> imgOutput(rows, cols);

// clang-format off
// clang-format on

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::medianBlur<WINDOW_SIZE, XF_BORDER_REPLICATE, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN,
                       XF_CV_DEPTH_OUT>(imgInput, imgOutput);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C