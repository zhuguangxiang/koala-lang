#!/bin/bash

KOALA_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
export KOALA_TARGET="${KOALA_TARGET:-DebugTest}"
export KOALA_HOME="$KOALA_ROOT"
export KOALA_PATH="$KOALA_ROOT/libs/:$KOALA_ROOT/test/:./"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$KOALA_ROOT/test/test_pkg"

case ":$PATH:" in
    *":$KOALA_ROOT/build/$KOALA_TARGET/bin:"*) ;;
    *) export PATH="$KOALA_ROOT/build/$KOALA_TARGET/bin:$PATH" ;;
esac
