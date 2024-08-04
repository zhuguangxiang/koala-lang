#!/bin/bash

mkdir -p build/release && cd build/release

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=release \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
cmake --build . --target test
cmake --build . --target check
