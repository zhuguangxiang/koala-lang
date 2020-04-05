#!/bin/bash

LLVM_INCLUDE=`llvm-config --includedir`
#echo $LLVM_INCLUDE
LLVM_LIBS=`llvm-config --libs core executionengine interpreter native mcjit`
#echo $LLVM_LIBS
gcc jit_llvm.c jit.c -I../../include -I$LLVM_INCLUDE $LLVM_LIBS -lkoala -static \
  -fPIC -shared -o ../jit.so