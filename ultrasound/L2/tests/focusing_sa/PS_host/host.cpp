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
#include <stdlib.h>
#include <fstream>
#include <iostream>

#include "graph.cpp"
#include "host.hpp"

// This is used for the PL Kernels
#include "xrt.h"
#include "experimental/xrt_kernel.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"

// aie
#define LENGTH 32
#define SPACE_DIMENSION 4

//
#define INPUT_SIZE_VECTOR 256
#define INPUT_SIZE_MATRIX INPUT_SIZE_VECTOR* SPACE_DIMENSION
#define OUTPUT_SIZE INPUT_SIZE_VECTOR

//#define INPUT_SIZE_DISTANCES INPUT_SIZE_VECTOR
#define INPUT_SIZE_DISTANCES (INPUT_SIZE_VECTOR / LENGTH) * SPACE_DIMENSION

#define INPUT_RANGE 4
#define OUTPUT_RANGE 1

class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

static std::vector<char> load_xclbin(xrtDeviceHandle device, const std::string& fnm) {
    if (fnm.empty()) throw std::runtime_error("No xclbin specified");

    // load bit stream
    std::ifstream stream(fnm);
    stream.seekg(0, stream.end);
    size_t size = stream.tellg();
    stream.seekg(0, stream.beg);

    std::vector<char> header(size);
    stream.read(header.data(), size);

    auto top = reinterpret_cast<const axlf*>(header.data());
    if (xrtDeviceLoadXclbin(device, top)) throw std::runtime_error("Xclbin loading failed");

    return header;
}

template <typename T>
T* data_loading(std::string filename, int& size) {
    T* buffer;

    std::ifstream infile(filename, std::ios::in);

    if (infile.is_open()) {
        // data loading
        buffer = new T[size];

        for (int i = 0; i < size; i++) {
            infile >> buffer[i];
        }
    } else {
        std::cout << "direction input is empty!" << std::endl;
    }

    infile.close();

    std::cout << "file:" << filename << " size:" << size << std::endl;
    return buffer;
}

int main(int argc, const char** argv) {
    //////////////////////////////////////////
    // input cmd parser
    //////////////////////////////////////////
    ArgParser parser(argc, argv);

    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:[-xclbin] xclbin path is not set!\n";
        return 1;
    }
    std::string data_path;
    if (!parser.getCmdOption("-data", data_path)) {
        std::cout << "ERROR:[-data] data path is not set!\n";
        return 1;
    }

    //////////////////////////////////////////
    // Open xclbin
    //////////////////////////////////////////
    auto dhdl = xrtDeviceOpen(0); // Open Device the local device
    if (dhdl == nullptr) throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
    auto xclbin = load_xclbin(dhdl, xclbin_path);
    auto top = reinterpret_cast<const axlf*>(xclbin.data());
    adf::registerXRT(dhdl, top->m_header.uuid);

    //////////////////////////////////////////
    // data loading
    //////////////////////////////////////////
    // global
    int sizeIn[4];
    int sizeOut[1];
    float* buf_mem[4];
    std::string file_set[4];

    // Input: image-points
    file_set[0] = data_path + "apo_ref_0.txt";
    file_set[1] = data_path + "xdc_def_0.txt";
    file_set[2] = data_path + "apo_ref_1.txt";
    file_set[3] = data_path + "xdc_def_1.txt";

    sizeIn[0] = 32;
    sizeIn[1] = 32;
    sizeIn[2] = 32;
    sizeIn[3] = 32;

    // mem data loading
    for (int i = 0; i < INPUT_RANGE; i++) {
        buf_mem[i] = data_loading<float>(file_set[i], sizeIn[i]);
    }

    // mem data output length define
    sizeOut[0] = 32; // focusing_sa

    //////////////////////////////////////////
    // input memory
    // Allocating the input size of sizeIn to MM2S
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////

    xrtBufferHandle in_bohdl_set[INPUT_RANGE];
    float* in_bomapped_set[INPUT_RANGE];

    for (int i = 0; i < INPUT_RANGE; i++) {
        in_bohdl_set[i] = xrtBOAlloc(dhdl, sizeIn[i] * sizeof(float), 0, 0);
        in_bomapped_set[i] = reinterpret_cast<float*>(xrtBOMap(in_bohdl_set[i]));
        memcpy(in_bomapped_set[i], buf_mem[i], sizeIn[i] * sizeof(float));
        std::cout << "Input memory" << i << " virtual addr 0x" << in_bomapped_set[i] << std::endl;
    }

    for (int i = 0; i < INPUT_RANGE; i++) {
        xrtBOSync(in_bohdl_set[i], XCL_BO_SYNC_BO_TO_DEVICE, sizeIn[i] * sizeof(float), 0);
    }

    //////////////////////////////////////////
    // output memory
    // Allocating the output size of sizeOut to S2MM
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////

    xrtBufferHandle out_bohdl_set[1];
    float* out_bomapped_set[1];

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        out_bohdl_set[i] = xrtBOAlloc(dhdl, sizeOut[i] * sizeof(float), 0, 0);
        out_bomapped_set[i] = reinterpret_cast<float*>(xrtBOMap(out_bohdl_set[i]));
        memset(out_bomapped_set[i], 0xABCDEF00, sizeOut[i] * sizeof(float));
        std::cout << "Output memory" << i << " virtual addr 0x " << out_bomapped_set[i] << std::endl;
    }

    //////////////////////////////////////////
    // mm2s ips
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    // set xrt kernels & run kernels for PL mm2s
    xrtKernelHandle mm2s_khdl_set[4];
    xrtRunHandle mm2s_rhdl_set[4];

    for (int i = 0; i < INPUT_RANGE; i++) {
        // get kl name
        char kl_name[256];
        sprintf(kl_name, "mm2s%d", i + 1);

        // Open mm2s PL kernels and Set arguments for run
        mm2s_khdl_set[i] = xrtPLKernelOpen(dhdl, top->m_header.uuid, kl_name);
        mm2s_rhdl_set[i] = xrtRunOpen(mm2s_khdl_set[i]);
        xrtRunSetArg(mm2s_rhdl_set[i], 0, in_bohdl_set[i]);
        xrtRunSetArg(mm2s_rhdl_set[i], 2, sizeIn[i]);
        xrtRunStart(mm2s_rhdl_set[i]);
    }
    std::cout << "input kernel complete" << std::endl;

    //////////////////////////////////////////
    // s2mm ips
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    // set xrt kernels & run kernels for PL s2mm
    xrtKernelHandle s2mm_khdl_set[6];
    xrtRunHandle s2mm_rhdl_set[6];

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        // gen kernel name
        char kl_name[256];
        sprintf(kl_name, "s2mm%d", i + 1);

        // Open s2mm PL kernels and Set arguments for run
        s2mm_khdl_set[i] = xrtPLKernelOpen(dhdl, top->m_header.uuid, kl_name);
        s2mm_rhdl_set[i] = xrtRunOpen(s2mm_khdl_set[i]);
        xrtRunSetArg(s2mm_rhdl_set[i], 0, out_bohdl_set[i]);
        xrtRunSetArg(s2mm_rhdl_set[i], 2, sizeOut[i]);
        xrtRunStart(s2mm_rhdl_set[i]);
    }
    std::cout << "output kernel complete" << std::endl;

    //////////////////////////////////////////
    // graph execution for AIE
    //////////////////////////////////////////
    std::cout << "graph init" << std::endl;
    b.init();

    b.run(1);
    std::cout << "graph run" << std::endl;

    b.end();
    std::cout << "graph end" << std::endl;

    //////////////////////////////////////////
    // wait for mm2s1 & mm2s2 done
    //////////////////////////////////////////

    for (int i = 0; i < INPUT_RANGE; i++) {
        // wait for run kernel done
        auto state = xrtRunWait(mm2s_rhdl_set[i]);
        std::cout << "mm2s" << i + 1 << " completed with status(" << state << ")\n";
        xrtRunClose(mm2s_rhdl_set[i]);
        xrtKernelClose(mm2s_khdl_set[i]);
    }
    std::cout << "mm2s wait complete" << std::endl;

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        auto state = xrtRunWait(s2mm_rhdl_set[i]);
        std::cout << "s2mm" << i + 1 << "completed with status(" << state << ")\n";
        xrtRunClose(s2mm_rhdl_set[i]);
        xrtKernelClose(s2mm_khdl_set[i]);
    }
    std::cout << "s2mm wait compete" << std::endl;

    //////////////////////////////////////////
    // wait for s2mm done
    //////////////////////////////////////////

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        xrtBOSync(out_bohdl_set[i], XCL_BO_SYNC_BO_FROM_DEVICE, sizeOut[i] * sizeof(float), 0);
    }

    //////////////////////////////////////////
    // Comparing the execution data to the golden data
    //////////////////////////////////////////

    int err_cnt = 0;
    std::string golden_file_set[OUTPUT_RANGE];
    golden_file_set[0] = data_path + "focusing_golden.txt";

    // loading golden data
    float* golden_buf_mem[OUTPUT_RANGE];
    int golden_sizeOut[OUTPUT_RANGE];

    golden_sizeOut[0] = 32;

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        golden_buf_mem[i] = data_loading<float>(golden_file_set[i], golden_sizeOut[i]);
    }

    for (int k = 0; k < OUTPUT_RANGE; k++) {
        for (int i = 0; i < sizeOut[k]; i++) {
            float result_tmp = (float)out_bomapped_set[k][i];
            float result_golden = golden_buf_mem[k][i];
            printf("i:%d, golden:%f result:%f\n", i, result_golden, result_tmp);
            if (result_tmp != result_golden) err_cnt++;
        }
    }

    //////////////////////////////////////////
    // clean up XRT
    //////////////////////////////////////////

    std::cout << "Releasing remaining XRT objects...\n";

    // intput handle and buffer clean
    for (int i = 0; i < INPUT_RANGE; i++) {
        xrtBOFree(in_bohdl_set[i]);
        delete[] buf_mem[i];
    }

    // output handle clean
    for (int i = 0; i < OUTPUT_RANGE; i++) {
        xrtBOFree(out_bohdl_set[i]);
    }

    // device close
    xrtDeviceClose(dhdl);

    // End
    if (!err_cnt)
        printf("Test Pass\n");
    else
        printf("Test Fail\n");

    return 0;
}
