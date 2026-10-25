#!/bin/bash
# Script to run benchmark tests

cd "$(dirname "$0")"

# Set KOALA_PATH to find standard libraries
export KOALA_PATH="libs/std/dist/:libs/koala/dist/:test/:"

for test_file in test/test-bench/*.kl; do
    if [ -f "$test_file" ]; then
        echo "=== Running $(basename $test_file) ==="
        koala --test "$test_file"
        echo ""
    fi
done
