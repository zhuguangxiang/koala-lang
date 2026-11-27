#!/bin/bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Rebuild the std library with the installed compiler.
(cd "$ROOT/libs" && wombat build std) || {
    echo "wombat build failed."
    exit 1
}

# std library tests, run by the installed koala
(cd "$ROOT/libs" && wombat test std) || {
    echo "wombat test failed."
    exit 1
}

echo ""
echo "=============================================="
echo " workspace: test (JIT disabled)"
echo "=============================================="

(cd "$ROOT/libs" && KOALA_JIT=0 wombat test std) || {
    echo "wombat test with JIT disabled failed."
    exit 1
}

# The demo workspace lives in examples/demo.
(cd "$ROOT/examples/demo" && wombat build) || {
    echo "wombat build failed."
    exit 1
}

(cd "$ROOT/examples/demo" && wombat test) || {
    echo "wombat test failed."
    exit 1
}

# no-jit
(cd "$ROOT/examples/demo" && KOALA_JIT=0 wombat test) || {
    echo "wombat test with JIT disabled failed."
    exit 1
}

echo "Done."

# cd build/debug

# cmake --build . --target install
