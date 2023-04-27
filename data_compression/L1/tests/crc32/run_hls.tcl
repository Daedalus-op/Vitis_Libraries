# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
# vitis hls makefile-generator v2.0.0

source settings.tcl

set PROJ "crc32.prj"
set SOLN "sol1"

if {![info exists CLKP]} {
  set CLKP 3.3
}

open_project -reset $PROJ

add_files "crc32_test.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/../security/L1/include -I${XF_PROJ_ROOT}/common/thirdParty/zlib"
add_files "${XF_PROJ_ROOT}/common/thirdParty/zlib-1.2.7/crc32.c" -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/../security/L1/include -I${XF_PROJ_ROOT}/common/thirdParty/zlib"
add_files -tb "crc32_test.cpp" -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/../security/L1/include -I${XF_PROJ_ROOT}/common/thirdParty/zlib"
add_files -tb sample.txt
set_top hls_crc32

open_solution -reset $SOLN



set_part $XPART
create_clock -period $CLKP

config_compile -pragma_strict_mode

if {$CSIM == 1} {
  csim_design -argv "${XF_PROJ_ROOT}/L1/tests/crc32/sample.txt" 
}

if {$CSYNTH == 1} {
  csynth_design
}

if {$COSIM == 1} {
  cosim_design -disable_dependency_check -argv "${XF_PROJ_ROOT}/L1/tests/crc32/sample.txt"
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

exit
