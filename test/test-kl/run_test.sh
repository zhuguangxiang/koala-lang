#!/bin/bash

# ---------------------------------------------------------
# Helper script to parse and run commands from .kl headers
# $1: Absolute path to koalac compiler
# $2: Absolute path to filecheck utility
# $3: Path to the .kl source file
# ---------------------------------------------------------

# Define ANSI color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color (Reset)

KOALAC_BIN="$1"
FILECHECK_BIN="$2"
SRC_FILE="$3"

FIRST_LINE=$(head -n 1 "$SRC_FILE")

# If the line starts with SKIP, exit with 0 (marked as pass/skipped)
if [[ "$FIRST_LINE" =~ ^([# /!]*SKIP:) ]]; then
    echo "Test Skipped: $SRC_FILE"
    exit 0
fi

# 1. Extract the command line from the first line of the source file.
# It strips comment prefixes like "// RUN: " or "# RUN: ".
RAW_LINE=$(echo "$FIRST_LINE" | sed -E 's|^([# /!]*RUN: )||')

# 2. Substitute placeholders with actual paths while preserving arguments.
# - 'koalac' is replaced by the compiler path.
# - 'filecheck' is replaced by the FileCheck path.
# - '%s' is replaced by the source file path.
FINAL_CMD=$(echo "$RAW_LINE" | \
    sed "s|koalac|$KOALAC_BIN|g" | \
    sed "s|filecheck|$FILECHECK_BIN|g" | \
    sed "s|%s|'$SRC_FILE'|g")

# 3. Validation and Execution logic
if [ -z "$FINAL_CMD" ]; then
    # Print error message in RED to stderr
    echo -e "${RED}Error: No valid 'RUN:' directive found in $SRC_FILE${NC}" >&2
    exit 1
else
    # Print the command being executed in GREEN
    echo -e "${GREEN}Executing: ${NC}$FINAL_CMD"

    # Run the synthesized command
    eval "$FINAL_CMD"
fi
