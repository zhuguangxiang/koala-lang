#!/bin/bash

mkdir -p build/debug && cd build/debug

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=debug \
    -DCMAKE_VERBOSE_MAKEFILE=OFF \
    -DSKIP_TESTS=OFF

cmake --build . --target install
