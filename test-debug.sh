#!/bin/bash

mkdir -p build/debug && cd build/debug

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=debug \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
cmake --build . --target test
cmake --build . --target check
