#!/bin/bash

# export PATH=$PATH:$(pwd)/build/DebugTest/bin
export KOALA_PATH="$(pwd)/libs/dist/:$(pwd)/test/:./"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/test/test_pkg
