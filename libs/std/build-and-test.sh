#!/bin/bash
#
# Build and test the workspace containing this script.
#
# `KOALA_PATH` is never exported. It is set only for the `wombat test`
# command, so the parent shell's environment is untouched and no
# workspace's `dist` leaks into another workspace's builds.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

DIST="$SCRIPT_DIR/dist"

echo "=============================================="
echo " workspace: build"
echo "=============================================="
wombat build

echo ""
echo "=============================================="
echo " workspace: test"
echo "=============================================="

# Per-command environment. Not exported, so this script leaves
# KOALA_PATH alone after it exits.
KOALA_PATH="$DIST/:$KOALA_PATH" wombat test
