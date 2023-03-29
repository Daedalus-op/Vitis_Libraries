/*
 * Copyright 2023-2024 Xilinx, Inc.
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

#define XF_NPPC XF_NPPC1 // XF_NPPC1 --1PIXEL , XF_NPPC2--2PIXEL ,XF_NPPC4--4 and XF_NPPC8--8PIXEL

#define XF_WIDTH 256  // MAX_COLS
#define XF_HEIGHT 256 // MAX_ROWS

//#define XF_BAYER_PATTERN XF_BAYER_RG // bayer pattern

#define T_8U 0
#define T_10U 0
#define T_12U 0
#define T_16U 1

#define XF_CCM_TYPE XF_CCM_bt2020_bt709

#if (T_16U || T_10U || T_12U)
#define CVTYPE unsigned short
#else
#define CVTYPE unsigned char
#endif

#if T_8U
#define XF_SRC_T XF_8UC1 // XF_8UC1
#define XF_LTM_T XF_8UC3 // XF_8UC3
#define XF_DST_T XF_8UC3 // XF_8UC3
#elif T_16U
#define XF_SRC_T XF_16UC1 // XF_8UC1
#define XF_LTM_T XF_8UC3  // XF_8UC3
#define XF_DST_T XF_16UC3 // XF_8UC3
#elif T_10U
#define XF_SRC_T XF_10UC1 // XF_8UC1
#define XF_LTM_T XF_8UC3  // XF_8UC3
#define XF_DST_T XF_10UC3 // XF_8UC3
#elif T_12U
#define XF_SRC_T XF_12UC1 // XF_8UC1
#define XF_LTM_T XF_8UC3  // XF_8UC3
#define XF_DST_T XF_12UC3 // XF_8UC3
#endif

#define NUM_STREAMS 4

#define STRM_HEIGHT 256
#define STRM1_ROWS STRM_HEIGHT
#define STRM2_ROWS STRM_HEIGHT
#define STRM3_ROWS STRM_HEIGHT
#define STRM4_ROWS STRM_HEIGHT

#if XF_HEIGHT == 256
#if STRM_HEIGHT == 128 // if XF_HEIGHT 1080 STRM_HEIGHT 256, Multi-slice
#define NUM_SLICES 2
#elif STRM_HEIGHT == 256 //  if XF_HEIGHT 1080 STRM_HEIGHT 1080, Single-slice
#define NUM_SLICES 1
#endif
#endif

#define SIN_CHANNEL_TYPE XF_8UC1
#define AEC_SIN_CHANNEL_TYPE XF_16UC1

#define USE_HDR_FUSION 0
#define USE_GTM 0
#define USE_LTM 1
#define USE_QnD 0
#define USE_RGBIR 1
#define USE_3DLUT 1
#define USE_DEGAMMA 1
#define USE_AEC 1

#if USE_HDR_FUSION
#define RD_MULT 2
#define RD_ADD 8

#else
#define RD_MULT 1
#define RD_ADD 0
#endif

#define WB_TYPE XF_WB_SIMPLE
#define DGAMMA_KP 8

#define AEC_EN 0

#define XF_AXI_GBR 1
#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64
#define LUT_PTR_WIDTH 128

#define NUM_V_BLANK_LINES 8
#define NUM_H_BLANK 8

#define XF_USE_URAM 0 // uram enable

#define MAX_HEIGHT STRM_HEIGHT * 2
#define MAX_WIDTH XF_WIDTH + NUM_H_BLANK

#define XF_CV_DEPTH_LEF 3
#define XF_CV_DEPTH_SEF 3
#define XF_CV_DEPTH_IN_0_1 3
#define XF_CV_DEPTH_IN_0_2 3
#define XF_CV_DEPTH_IN_1 3
#define XF_CV_DEPTH_IN_2 3
#define XF_CV_DEPTH_IN_3 3
#define XF_CV_DEPTH_IN_4 3
#define XF_CV_DEPTH_IN_5 3
#define XF_CV_DEPTH_IN_6 3
#define XF_CV_DEPTH_IN_7 3
#define XF_CV_DEPTH_IN_8 3
#define XF_CV_DEPTH_IN_9 3
#define XF_CV_DEPTH_OUT_0 3
#define XF_CV_DEPTH_OUT_1 3
#define XF_CV_DEPTH_OUT_2 3
#define XF_CV_DEPTH_OUT_3 3
#define XF_CV_DEPTH_OUT_4 3
#define XF_CV_DEPTH_OUT_5 3
#define XF_CV_DEPTH_OUT_6 3
#define XF_CV_DEPTH_OUT_7 3
#define XF_CV_DEPTH_3dlut 3
#define XF_CV_DEPTH_OUT_IR 3
#define XF_CV_DEPTH_3XWIDTH 3 * XF_WIDTH
