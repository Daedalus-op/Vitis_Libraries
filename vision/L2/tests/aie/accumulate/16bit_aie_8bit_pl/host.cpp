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

#define PROFILE
#include "adf.h"
#include <adf/adf_api/XRTConfig.h>
#include <chrono>
#include <common/xf_aie_sw_utils.hpp>
#include <common/xfcvDataMovers.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xrt/experimental/xrt_kernel.h>
#include <xaiengine.h>

#include "graph.cpp"

int run_opencv_ref(cv::Mat& srcImage1, cv::Mat& srcImage2, cv::Mat& dstRefImage) {
    cv::Mat ocv_ref_32f(srcImage1.rows, srcImage1.cols, CV_32F);
    dstRefImage.create(srcImage1.rows, srcImage1.cols, CV_8UC1);
    srcImage2.convertTo(ocv_ref_32f, CV_32F);
    cv::accumulate(srcImage1, ocv_ref_32f, cv::noArray());
    ocv_ref_32f.convertTo(dstRefImage, CV_8U);
    return 0;
}

/*
 ******************************************************************************
 * Top level executable
 ******************************************************************************
 */

int main(int argc, char** argv) {
    try {
        if (argc != 4) {
            std::stringstream errorMessage;
            errorMessage << argv[0] << " <xclbin> <inputImage1> <inputImage2>";
            std::cerr << errorMessage.str();
            throw std::invalid_argument(errorMessage.str());
        }

        const char* xclBinName = argv[1];
        //////////////////////////////////////////
        // Read image from file and resize
        //////////////////////////////////////////
        cv::Mat srcImage1, srcImage2;
        srcImage1 = cv::imread(argv[2], 0);
        srcImage2 = cv::imread(argv[3], 0);
        std::cout << "Image1 size" << std::endl;
        std::cout << srcImage1.rows << std::endl;
        std::cout << srcImage1.cols << std::endl;
        std::cout << srcImage1.elemSize() << std::endl;
        std::cout << "Image2 size" << std::endl;
        std::cout << srcImage2.rows << std::endl;
        std::cout << srcImage2.cols << std::endl;
        std::cout << srcImage2.elemSize() << std::endl;
        std::cout << "Image size (end)" << std::endl;
        int op_width = srcImage1.cols;
        int op_height = srcImage1.rows;

        //////////////////////////////////////////
        // Run opencv reference test (accumulate design)
        //////////////////////////////////////////
        cv::Mat dstRefImage;
        run_opencv_ref(srcImage1, srcImage2, dstRefImage);

        // Initializa device
        xF::deviceInit(xclBinName);

        // Load image
        void* srcData1 = nullptr;
        void* srcData2 = nullptr;
        xrtBufferHandle src_hndl1 = xrtBOAlloc(xF::gpDhdl, (srcImage1.total() * srcImage1.elemSize()), 0, 0);
        xrtBufferHandle src_hndl2 = xrtBOAlloc(xF::gpDhdl, (srcImage2.total() * srcImage2.elemSize()), 0, 0);
        srcData1 = xrtBOMap(src_hndl1);
        srcData2 = xrtBOMap(src_hndl2);
        memcpy(srcData1, srcImage1.data, (srcImage1.total() * srcImage1.elemSize()));
        memcpy(srcData2, srcImage2.data, (srcImage2.total() * srcImage2.elemSize()));

        // Allocate output buffer
        void* dstData = nullptr;
        xrtBufferHandle dst_hndl =
            xrtBOAlloc(xF::gpDhdl, (op_height * op_width * srcImage1.elemSize()), 0, 0); // '2' for unsigned short type
        dstData = xrtBOMap(dst_hndl);
        cv::Mat dst(op_height, op_width, CV_8UC1, dstData);

        xF::xfcvDataMovers<xF::TILER, int16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> tiler1(0, 0);
        xF::xfcvDataMovers<xF::TILER, int16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> tiler2(0, 0);
        xF::xfcvDataMovers<xF::STITCHER, int16_t, TILE_HEIGHT, TILE_WIDTH, VECTORIZATION_FACTOR> stitcher;

        std::cout << "Graph init. This does nothing because CDO in boot PDI already configures AIE.\n";
        accum_graph.init();

        START_TIMER
        tiler1.compute_metadata(srcImage1.size());
        tiler2.compute_metadata(srcImage1.size());
        STOP_TIMER("Meta data compute time")

        //@{
        START_TIMER
        auto tiles_sz = tiler1.host2aie_nb(src_hndl1, srcImage1.size());
        tiler2.host2aie_nb(src_hndl2, srcImage2.size());
        stitcher.aie2host_nb(dst_hndl, dst.size(), tiles_sz);

        std::cout << "Graph run(" << (tiles_sz[0] * tiles_sz[1]) << ")\n";
        accum_graph.run(tiles_sz[0] * tiles_sz[1]);
        accum_graph.wait();
        std::cout << "Graph run complete\n";

        tiler1.wait();
        tiler2.wait();
        std::cout << "Data transfer complete (Tiler)\n";

        stitcher.wait();
        STOP_TIMER("Total time to process frame")
        std::cout << "Data transfer complete (Stitcher)\n";
        //@}

        // Analyze output {
        std::cout << "Analyzing diff\n";
        cv::Mat diff;
        cv::absdiff(dstRefImage, dst, diff);
        cv::imwrite("ref.png", dstRefImage);
        cv::imwrite("aie.png", dst);
        cv::imwrite("diff.png", diff);

        float err_per;
        analyzeDiff(diff, 2, err_per);
        if (err_per > 0.0f) {
            std::cerr << "Test failed" << std::endl;
            exit(-1);
        }
        //}
        accum_graph.end();
        std::cout << "Test passed" << std::endl;
        return 0;
    } catch (std::exception& e) {
        const char* errorMessage = e.what();
        std::cerr << "Exception caught: " << errorMessage << std::endl;
        exit(-1);
    }
}
