#!/bin/bash

# export PATH=$PATH:$(pwd)/
export KOALA_PATH="$(pwd)/libs/std/dist/:$(pwd)/libs/koala/dist/:$(pwd)/test/:$(pwd)/"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/james/.local/lib:$(pwd)/libs/koala/dist/koala/:$(pwd)/test/test_pkg

# Tab completion for wombat
if [ -f "$(pwd)/tools/completions/wombat.bash" ]; then
    source "$(pwd)/tools/completions/wombat.bash"
fi
