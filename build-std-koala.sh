#!/bin/bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Rebuild the std and koala libraries with the installed compiler, then install
# their new dist directories.
(cd "$ROOT/libs" && wombat build std && wombat build koala) || {
    echo "wombat build failed."
    exit 1
}

# std and koala library tests, run by the installed koala
(cd "$ROOT/libs" && wombat test std && wombat test koala) || {
    echo "wombat test failed."
    exit 1
}

echo ""
echo "=============================================="
echo " workspace: test (JIT disabled)"
echo "=============================================="

(cd "$ROOT/libs" && KOALA_JIT=0 wombat test std && KOALA_JIT=0 wombat test koala) || {
    echo "wombat test with JIT disabled failed."
    exit 1
}

echo "Done."

cd build/debug

cmake --build . --target install
