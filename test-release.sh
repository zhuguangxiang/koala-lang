#!/bin/bash

mkdir -p build/release && cd build/release

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=release \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all
# cmake --build . --target koala-tests
cmake --build . --target lit-tests
# C unit tests (ctest `c_` prefix, e.g. c_ring), lives in test/test-c
# cmake --build . --target koala-c-tests

# ctest -R "max" --output-on-failure

echo "Done."
