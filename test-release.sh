#!/bin/bash

mkdir -p build/Release && cd build/Release

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
cmake --build . --target test
cmake --build . --target check
