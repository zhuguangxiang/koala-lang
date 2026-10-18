#!/bin/bash

source ./completions/wombat.bash
export PATH=$PATH:$(pwd)/
export KOALA_PATH="$(pwd)/libs/dist/:$(pwd)/test/:./:$(pwd)/"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/test/test_pkg

# Tab completion for wombat
if [ -f "$(pwd)/completions/wombat.bash" ]; then
    source "$(pwd)/completions/wombat.bash"
fi
