#!/bin/bash

source ./completions/wombat.bash
export PATH=$PATH:$(pwd)/
export KOALA_PATH="$(pwd)/libs/dist/:$(pwd)/test/:./"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/test/test_pkg
