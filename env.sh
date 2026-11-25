#!/bin/bash

# export PATH=$PATH:$(pwd)/
# Installed third-party packages (koala-libs) live under the stable `koala`
# symlink; add them to KOALA_PATH (imports) and their native dir to
# LD_LIBRARY_PATH (dlopen resolves libkoala_*.so by basename).
export KOALA_PATH="$(pwd)/libs/std/dist/:$(pwd)/test/:$HOME/.local/lib/koala/lib/koala-packages/:$(pwd)/"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/.local/lib:$HOME/.local/lib/koala/lib/koala-packages/koala:$(pwd)/test/test_pkg

# Tab completion for wombat
if [ -f "$(pwd)/tools/completions/wombat.bash" ]; then
    source "$(pwd)/tools/completions/wombat.bash"
fi
