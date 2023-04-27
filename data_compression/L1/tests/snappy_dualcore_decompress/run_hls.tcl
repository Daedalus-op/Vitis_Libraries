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
set DIR_NAME "snappy_dualcore_decompress"
set DESIGN_PATH "${XF_PROJ_ROOT}/L1/tests/${DIR_NAME}"
set PROJ "snappy_decompress_test.prj"
set SOLN "sol1"
set CLKP 3.3

# Create a project
open_project -reset $PROJ

# Add design and testbench files
add_files $XF_PROJ_ROOT/common/libs/logger/logger.cpp -cflags "-I${XF_PROJ_ROOT}/common/libs/logger"
add_files $XF_PROJ_ROOT/common/libs/cmdparser/cmdlineparser.cpp -cflags "-I${XF_PROJ_ROOT}/common/libs/cmdparser -I${XF_PROJ_ROOT}/common/libs/logger"
add_files snappy_decompress_test.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/common/libs/cmdparser -I${XF_PROJ_ROOT}/common/libs/logger -I${XF_PROJ_ROOT}/../security/L1/include -DMULTIPLE_BYTES=8"

add_files -tb ${XF_PROJ_ROOT}/common/libs/logger/logger.cpp -cflags "-I${XF_PROJ_ROOT}/common/libs/logger"
add_files -tb ${XF_PROJ_ROOT}/common/libs/cmdparser/cmdlineparser.cpp -cflags "-I${XF_PROJ_ROOT}/common/libs/cmdparser -I${XF_PROJ_ROOT}/common/libs/logger"
add_files -tb snappy_decompress_test.cpp -cflags "-I${XF_PROJ_ROOT}/L1/include/hw -I${XF_PROJ_ROOT}/common/libs/cmdparser -I${XF_PROJ_ROOT}/common/libs/logger -I${XF_PROJ_ROOT}/../security/L1/include -DMULTIPLE_BYTES=8"

# Set the top-level function
set_top snappyMultiCoreDec

# Create a solution
open_solution -reset $SOLN

# Define technology and clock rate
set_part {xcu200}
create_clock -period $CLKP

config_compile -pragma_strict_mode

if {$CSIM == 1} {
  csim_design -O -argv "-d ${DESIGN_PATH}/sample.txt.snappy"
}

if {$CSYNTH == 1} {
  csynth_design  
}

if {$COSIM == 1} {
  cosim_design -disable_dependency_check -O -argv "-d ${DESIGN_PATH}/sample.txt.snappy"
}

if {$VIVADO_SYN == 1} {
  export_design -flow syn -rtl verilog
}

if {$VIVADO_IMPL == 1} {
  export_design -flow impl -rtl verilog
}

if {$QOR_CHECK == 1} {
  puts "QoR check not implemented yet"
}
exit
