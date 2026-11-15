#!/bin/bash
#
# This file is part of the koala project with MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
#
# Run the wsdeque test under ThreadSanitizer.
# Usage: test/test-c/tsan_wsdeque.sh [rounds]

set -e

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

ROUNDS=${1:-10}

gcc -std=gnu17 -g -O1 -fsanitize=thread -fno-omit-frame-pointer \
    -I"$ROOT/include/common" \
    "$ROOT/test/test-c/test_wsdeque.c" \
    "$ROOT/src/common/wsdeque.c" \
    "$ROOT/src/common/mm.c" \
    "$ROOT/src/common/log.c" \
    -o "$OUT/test_wsdeque" -lpthread -lm

for i in $(seq 1 "$ROUNDS"); do
    echo "=== round $i/$ROUNDS ==="
    TSAN_OPTIONS="halt_on_error=1 second_deadlock_stack=1" \
        "$OUT/test_wsdeque" | tail -1
done

echo "tsan: all $ROUNDS rounds passed"
