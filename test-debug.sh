#!/bin/bash

mkdir -p build/Debug && cd build/Debug

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
cmake --build . --target test
cmake --build . --target check
