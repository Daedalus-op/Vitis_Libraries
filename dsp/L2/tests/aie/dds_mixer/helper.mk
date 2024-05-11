#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

###############################################################################
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################
# Values for diff_tolerance are obtained experimentally. They are non-zero for streaming cases because
# number of lanes differs between reference model and uut. They are non-zero for ssr cases since uut and reference
# model split the lookup angle into differet sub-angles. Sincos lookup values of these sub-angles suffer from information
# loss due to rounding. Further, there is a rounding error from their multiplication.

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
DIFF_MODE          = PERCENT
DIFF_TOLERANCE     = 0.01
CC_TOLERANCE       = 0
PARAM_MAP = DATA_TYPE $(DATA_TYPE) \
			MIXER_MODE $(MIXER_MODE) \
			USE_PHASE_RELOAD $(USE_PHASE_RELOAD) \
			P_API $(P_API) \
			UUT_SSR $(UUT_SSR) \
			INPUT_WINDOW_VSIZE $(INPUT_WINDOW_VSIZE) \
			ROUND_MODE $(ROUND_MODE) \
			SAT_MODE $(SAT_MODE) \
			NITER $(NITER) \
			INITIAL_DDS_OFFSET $(INITIAL_DDS_OFFSET) \
			DDS_PHASE_INC $(DDS_PHASE_INC) \
			DATA_SEED $(DATA_SEED) \
			DATA_STIM_TYPE $(DATA_STIM_TYPE) \
			DIFF_MODE $(DIFF_MODE) \
			DIFF_TOLERANCE $(DIFF_TOLERANCE)
       

# ref model and uut use different base angles when SSR > 1 and thus are prone to different bit errors
# set DIFF_TOLERANCE = 4 when DATA_TYPE = cint32 and UUT_SSR > 1 
ifeq ($(DATA_TYPE), cint32)
	ifneq ($(UUT_SSR), 1)
		DIFF_TOLERANCE := 4
		DIFF_MODE := ABS
	endif
endif
# set DIFF_TOLERANCE = 4 when DATA_TYPE = cint16, INITIAL_DDS_OFFSET != 0 and UUT_SSR != 1 
ifeq ($(DATA_TYPE), cint16)
	ifneq ($(INITIAL_DDS_OFFSET), 0)
		ifneq ($(UUT_SSR), 1)
			DIFF_TOLERANCE := 4
			DIFF_MODE := ABS
		endif
	endif
endif

ifeq ($(DATA_TYPE), float)
	CC_TOLERANCE 	  := 0.01
else ifeq ($(DATA_TYPE), cfloat)
	CC_TOLERANCE 	  := 0.01
endif

HELPER_CUR_DIR ?= .

diff:
	@echo helper.mk stage: diff
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) $(DIFF_MODE)
	
gen_input:
	@echo helper.mk stage:  gen_input
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 $(DATA_TYPE) 0 1

ssr_split:
	@echo helper.mk stage:  ssr_split
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --split --dual 0 -k 0 -w $(INPUT_WINDOW_VSIZE)

ssr_zip:
	@echo helper.mk stage:  ssr_split
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --zip --dual 0 -k 0 -w $(INPUT_WINDOW_VSIZE)

get_status:
	@echo helper.mk stage:  get_status
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) USE_OUTPUTS_IF_NO_INPUTS

get_stats:
	@echo helper.mk stage:  get_stats
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) 1 $(STATUS_FILE) ./aiesimulator_output dds $(NITER)

get_theoretical_min:
	@echo helper.mk stage:  get_theoretical_min
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_dds_theoretical_min.tcl $(DATA_TYPE) $(MIXER_MODE) $(INPUT_WINDOW_VSIZE) $(UUT_SSR) $(STATUS_FILE) $(UUT_KERNEL)

harvest_mem:
	@echo helper.mk stage:  harvest_mem
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts