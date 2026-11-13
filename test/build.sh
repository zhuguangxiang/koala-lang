#!/bin/bash

koala -c --package-name=test_pkg/pkg1 test_pkg/pkg1
koala -c --package-name=test_pkg/pkg2 test_pkg/pkg2
koala -c --package-name=test_pkg/pkg3 test_pkg/pkg3
koala -c --package-name=test_pkg/pkg4 test_pkg/pkg4
koala -c --package-name=test_pkg/pkg_chain test_pkg/pkg_chain
koala -c --package-name=test_pkg/pkg_intf test_pkg/pkg_intf
koala -c --package-name=test_pkg/pkg_field test_pkg/pkg_field
koala -c --package-name=test_pkg/pkg_no_init test_pkg/pkg_no_init
koala -c --package-name=test_pkg/pkg_generic_method test_pkg/pkg_generic_method
koala -c --package-name=test_pkg/lambda_no_capture_dep test_pkg/lambda_no_capture_dep
koala -c --package-name=test_pkg/pkg_const_then_let test_pkg/pkg_const_then_let
koala -c --package-name=test_pkg/pkg_bytes_default test_pkg/pkg_bytes_default
koala -c --package-name=test_pkg/pkg_with test_pkg/pkg_with
koala -c --package-name=test_pkg/enum_cross test_pkg/enum_cross
koala -c --package-name=test_pkg/pkg_const_bytes test_pkg/pkg_const_bytes

cd test_pkg
gcc -fPIC -shared pkg1_native/pkg1_native_impl.c -o libpkg1_native.so -I../../include/runtime -I../../include/common -L../../build/DebugTest/lib/ -g
