#!/bin/bash
# Script to build and run tests in DebugTest configuration

mkdir -p build/DebugTest && cd build/DebugTest

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=DebugTest \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
# cmake --build . --target koala-tests
cmake --build . --target lit-tests

# ctest -R "max" --output-on-failure

echo "Done."
