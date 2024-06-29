#!/bin/bash

mkdir -p build/Release && cd build/Release

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
