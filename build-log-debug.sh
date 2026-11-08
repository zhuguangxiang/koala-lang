#!/bin/bash

mkdir -p build/debug_log && cd build/debug_log

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=debug_log \
    -DCMAKE_VERBOSE_MAKEFILE=OFF \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
# cmake --build . --target install
