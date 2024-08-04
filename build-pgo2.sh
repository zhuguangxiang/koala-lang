#!/bin/bash

mkdir -p build/pgo && cd build/pgo

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=pgo2 \
    -DCMAKE_VERBOSE_MAKEFILE=ON \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
cmake --build . --target test
cmake --build . --target check
