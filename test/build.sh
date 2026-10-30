#!/bin/bash

koala -c --package-name=test_pkg/pkg1 test_pkg/pkg1
koala -c --package-name=test_pkg/pkg2 test_pkg/pkg2
koala -c --package-name=test_pkg/pkg3 test_pkg/pkg3
koala -c --package-name=test_pkg/pkg_chain test_pkg/pkg_chain
koala -c --package-name=test_pkg/pkg_intf test_pkg/pkg_intf

cd test_pkg
gcc -fPIC -shared pkg1_native/pkg1_native_impl.c -o libpkg1_native.so -I../../include/runtime -I../../include/common -L../../build/DebugTest/lib/ -g
