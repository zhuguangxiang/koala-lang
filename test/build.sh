#!/bin/bash

koala -c test_pkg/pkg1 --package-name=test_pkg/pkg1
koala -c test_pkg/pkg2 --package-name=test_pkg/pkg2
koala -c test_pkg/pkg_chain --package-name=test_pkg/pkg_chain

cd test_pkg
gcc -fPIC -shared pkg1_native/pkg1_native_impl.c -o libpkg1_native.so -I../../include/runtime -I../../include/common -L../../build/DebugTest/lib/ -lkoala -g
