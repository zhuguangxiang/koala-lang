#!/bin/bash

mkdir -p build/$1 && cd build/$1

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=$1 \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
# cmake --build . --target test
# cmake --build . --target check