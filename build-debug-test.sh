#!/bin/bash

mkdir -p build/DebugTest && cd build/DebugTest

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=DebugTest \
    -DCMAKE_VERBOSE_MAKEFILE=OFF \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
cmake --build . --target install

cd ../..

koala -c --package-name=wombat_build wombat_build.kl
koala -c --package-name=wombat_test wombat_test.kl
