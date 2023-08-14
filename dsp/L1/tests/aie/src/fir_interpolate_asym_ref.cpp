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
#include "aie_api/aie_adf.hpp"
#include "fir_interpolate_asym_ref.hpp"
#include "fir_ref_utils.hpp"
#define _DSPLIB_FIR_INTERPOLATE_ASYM_REF_DEBUG_
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_asym {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
void filter_ref(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& inWindow,
                output_circular_buffer<TT_DATA>& outWindow,
                const TT_COEFF (&taps)[TP_FIR_LEN]) {
    const unsigned int shift = TP_SHIFT;
    const unsigned int kFirLen = TP_FIR_LEN / TP_INTERPOLATE_FACTOR;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[kFirLen];
    TT_DATA accum_srs;

    auto inItr = ::aie::begin_random_circular(inWindow);
    auto outItr = ::aie::begin_random_circular(outWindow);
    int q = 0;

    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    inItr += kFirMarginOffset; // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        for (unsigned int j = 0; j < kFirLen; ++j) {
            d_in[j] = *inItr++;
        }

        for (int k = TP_INTERPOLATE_FACTOR - 1; k >= 0; --k) {
            accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
            for (unsigned int j = 0; j < kFirLen; ++j) {
                multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], taps[j * TP_INTERPOLATE_FACTOR + k]);
            }
            // prior to output, the final accumulated value must be downsized to the same type
            // as was input. To do this, the final result is rounded, saturated and shifted down
            roundAcc(TP_RND, shift, accum);
            saturateAcc(accum);
            accum_srs = castAcc(accum);
            *outItr++ = accum_srs;
        }
        // Revert data pointer for next sample
        inItr -= kFirLen - 1;
    }
};

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API>
void fir_interpolate_asym_ref<TT_DATA,
                              TT_COEFF,
                              TP_FIR_LEN,
                              TP_INTERPOLATE_FACTOR,
                              TP_SHIFT,
                              TP_RND,
                              TP_INPUT_WINDOW_VSIZE,
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_API>::
    filter(input_circular_buffer<TT_DATA,
                                 extents<inherited_extent>,
                                 margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& inWindow,
           output_circular_buffer<TT_DATA>& outWindow) {
    //    firHeaderReload<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_USE_COEFF_RELOAD>(inWindow,
    //    m_internalTaps); //coeffs on header has been dropped.
    filter_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE>(
        inWindow, outWindow, m_internalTaps);
};

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API>
void fir_interpolate_asym_ref<TT_DATA,
                              TT_COEFF,
                              TP_FIR_LEN,
                              TP_INTERPOLATE_FACTOR,
                              TP_SHIFT,
                              TP_RND,
                              TP_INPUT_WINDOW_VSIZE,
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS,
                              TP_API>::
    filterRtp(input_circular_buffer<TT_DATA,
                                    extents<inherited_extent>,
                                    margin<fnFirMargin<TP_FIR_LEN / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& inWindow,
              output_circular_buffer<TT_DATA>& outWindow,
              const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    // Coefficient reload
    for (int i = 0; i < TP_FIR_LEN; i++) {
        m_internalTaps[i] = inTaps[TP_FIR_LEN - 1 - i];
    }
    filter_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE>(
        inWindow, outWindow, m_internalTaps);
};
}
}
}
}
}
