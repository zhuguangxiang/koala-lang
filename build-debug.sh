#!/bin/bash

mkdir -p build/Debug && cd build/Debug

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_VERBOSE_MAKEFILE=OFF \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
