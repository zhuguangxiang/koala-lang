#!/bin/bash

# set -e

mkdir -p build/coverage && cd build/coverage

cmake -G Ninja ../.. \
    -DCMAKE_BUILD_TYPE=coverage \
    -DCMAKE_VERBOSE_MAKEFILE=OFF \
    -DSKIP_TESTS=OFF

cmake --build . --target clean
cmake --build . --target all

find . -name '*.gcda' -delete

cmake --build . --target lit-tests

gcovr -r ../.. \
  --object-directory=../ \
  --filter="../../src/" \
  --filter="../../include/" \
  --filter="../../libs/" \
  --exclude ".*koala_lex.*" \
  --exclude ".*koala_yacc.*" \
  --html-details \
  -o index.html \
  .
