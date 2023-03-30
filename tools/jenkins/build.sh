#!/bin/bash

# Do not clean the toolchain if a quick build is requested
if [ $# -eq 0 ] || [ $1 != "quick" ]
then
   # Clean the toolchain
   rm -rf ./tools/toolchain
fi

# Build the AMD Version of the code
export CC="/usr/bin/gcc-4.8"
export CXX="/usr/bin/g++-4.8"
mkdir -p .build_amd
pushd .build_amd
cmake ../
if [ $# -eq 0 ] || [ $1 != "quick" ]
then
   # Run the build and unit tests
   make && ctest -T MemCheck || make test
else
   # Do not run the unit tests for the quick build
   # AMD build currently broken, will fix in FB67334, then this make should be re-enabled
   #make -j8
   : ;
fi
popd

# Build the ARM Version of the code
export ARCH_BUILD="armel"

if [ ! -d "./tools/toolchain" ];
then
   echo "Unpacking build tools"
   tar -xjf ./tools/gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2 -C ./tools
   mv ./tools/gcc-arm-none-eabi-5_2-2015q4/ ./tools/toolchain
fi

export PATH=`pwd`/tools/toolchain/bin:$PATH
echo "Path is now set to $PATH"

unset CC
unset CXX
mkdir -p .build_armel
pushd .build_armel
cmake -DCMAKE_TOOLCHAIN_FILE=../tools/cmake/armel.cmake ../
/bin/bash -c make -j8
popd
