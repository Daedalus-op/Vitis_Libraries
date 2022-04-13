#!/bin/bash
EXE_FILE=$1
LIB_PROJ_ROOT=$2
XCLBIN_FILE=$3
echo "XCL_MODE=${XCL_EMULATION_MODE}"
if [ "${XCL_EMULATION_MODE}" != "hw_emu" ] 
then
    cp $LIB_PROJ_ROOT/common/data/sample.txt ./sample_run.txt
    cp $LIB_PROJ_ROOT/common/data/test.list ./test.list
    find ./reports/ -type f | xargs cat >> ./sample_run.txt
    split -b 100M ./sample_run.txt ./segment
    mv ./segmentaa ./sample_run.txt 
    rm -f segment*
    
    echo -e "\n\n-----------Running both Compression and Decompression for large file-----------\n"
    cmd1="$EXE_FILE -l ./test.list -mcr 20 -xbin $XCLBIN_FILE"
    echo $cmd1
    $cmd1

fi
