#!/usr/bin/env bash
#
# demo/chat pty + non-tty end-to-end smoke.
#
# `wombat test chat` covers the pure renderers. This script drives the built
# artifact through a real pty (script(1)) so the chrome, readline input area
# and scroll regions are exercised too:
#   24 rows : full TUI (hint row present)
#   10 rows : smallest TUI that still has one transcript content row
#    9 rows : plain fallback, no invalid scroll region
#   24 -> 9 : live resize falls back and replays the transcript
#   pipe    : true non-tty output, no ANSI and no boxes
#
# Every run uses a temporary working directory, so chat_sessions/ and the
# history file never touch the repository.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHAT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
DEMO_DIR="$(cd "$CHAT_DIR/.." && pwd)"
ROOT="$(cd "$DEMO_DIR/.." && pwd)"

CHAT="$DEMO_DIR/dist/demo/chat.klc"

if [[ ! -f "$CHAT" ]]; then
    echo "test_pty: missing $CHAT; run 'cd demo && wombat build chat'" >&2
    exit 2
fi
if ! command -v script >/dev/null 2>&1; then
    echo "test_pty: SKIP (script(1) not found)"
    exit 0
fi
KOALA_BIN="$(command -v koala || true)"
if [[ -z "$KOALA_BIN" ]]; then
    echo "test_pty: koala not found in PATH; source env.sh first" >&2
    exit 2
fi

TMP="$(mktemp -d /tmp/koala-chat-pty.XXXXXX)"
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/config.json" <<JSON
{"model":"auto","base_url":"http://127.0.0.1:1/v1","history_file":"$TMP/history","max_code_bytes":1000,"temperature":0.5,"max_tokens":100,"context_window":4096,"system_prompt":"s","stream_delay_ms":0}
JSON

if [[ -z "${KOALA_PATH:-}" ]]; then
    export KOALA_PATH="$DEMO_DIR/dist/:$ROOT/libs/std/dist/:$ROOT/libs/koala/dist/:$ROOT"
fi
export OPENAI_COMPAT_API_KEY=x
export TERM=xterm-256color
export CONFIG="$TMP/config.json"
export KOALA_BIN CHAT

fails=0
pass() { printf 'ok - %s\n' "$*"; }
fail() { printf 'not ok - %s\n' "$*" >&2; fails=$((fails + 1)); }

# Run the artifact in a pty of `rows` x 80 and feed `input` on stdin.
run_pty() {
    local rows="$1"
    local input="$2"
    local out="$3"
    local runner="$TMP/run-$rows.sh"
    cat > "$runner" <<EOF
#!/bin/sh
stty rows $rows cols 80
exec "\$KOALA_BIN" "\$CHAT" --config "\$CONFIG"
EOF
    chmod +x "$runner"
    ( cd "$TMP" && sleep 0.5; printf '%s' "$input" ) |
        timeout 20 script -qec "$runner" "$out" >/dev/null 2>&1
}

# Start a pty and assert the UI shape for one terminal height.
check_pty() {
    local rows="$1"
    local out="$TMP/out-$1"
    if ! run_pty "$rows" $'/help\r/quit\r' "$out"; then
        fail "pty $rows rows exited non-zero or timed out"
        return
    fi
    if grep -aFq $'\x1b[2;1r' "$out"; then
        fail "pty $rows rows emitted invalid scroll region"
        return
    fi
    if [[ "$rows" -lt 10 ]]; then
        # `/help` also contains the words "Enter send", so match the full
        # hint row rather than the words alone.
        if grep -aFq 'Chat with auto. |' "$out" && ! grep -aFq 'Enter send · Ctrl-J newline · Tab complete' "$out"; then
            pass "pty $rows rows fell back to plain output"
        else
            fail "pty $rows rows did not use the plain fallback"
        fi
    else
        if grep -aFq 'Chat with auto.' "$out" && grep -aFq 'Enter send · Ctrl-J newline · Tab complete' "$out"; then
            pass "pty $rows rows kept the TUI chrome"
        else
            fail "pty $rows rows lost the TUI chrome"
        fi
    fi
}

check_pty 24
check_pty 10
check_pty 9

# Shrink a live 24-row TUI to 9 rows: the next poll must leave the chrome,
# replay the transcript and never set an invalid scroll region.
check_pty_resize() {
    local out="$TMP/out-resize"
    local runner="$TMP/run-resize.sh"
    cat > "$runner" <<EOF
#!/bin/sh
stty rows 24 cols 80
( sleep 1; stty rows 9 cols 80 < /dev/tty ) &
exec "\$KOALA_BIN" "\$CHAT" --config "\$CONFIG"
EOF
    chmod +x "$runner"
    if ! ( cd "$TMP" && sleep 2.5; printf '/quit\r' ) |
        timeout 20 script -qec "$runner" "$out" >/dev/null 2>&1; then
        fail "pty resize run exited non-zero or timed out"
        return
    fi
    if grep -aFq $'\x1b[2;1r' "$out"; then
        fail "pty resize emitted invalid scroll region"
        return
    fi
    # once in the initial TUI frame, once replayed by the plain fallback
    local replays
    replays="$(grep -aoF 'Chat with auto.' "$out" | wc -l)"
    if [[ "$replays" -ge 2 ]]; then
        pass "pty 24 -> 9 rows fell back without an invalid scroll region"
    else
        fail "pty 24 -> 9 rows did not replay the transcript"
    fi
}

check_pty_resize

# A real pipe has no tty: no cursor escapes, boxes, or hint row.
if ( cd "$TMP" && printf '/quit\n' | timeout 20 "$KOALA_BIN" "$CHAT" --config "$TMP/config.json" > "$TMP/non-tty.out" 2>&1 ); then
    if grep -aFq 'Chat with auto. |' "$TMP/non-tty.out" && ! grep -aFq $'\x1b' "$TMP/non-tty.out" && ! grep -aFq 'Enter send' "$TMP/non-tty.out"; then
        pass "non-tty output is plain"
    else
        fail "non-tty output contains ANSI or TUI chrome"
    fi
else
    fail "non-tty run exited non-zero"
fi

if [[ "$fails" -ne 0 ]]; then
    echo "test_pty: $fails check(s) failed" >&2
    exit 1
fi
echo "test_pty: all checks passed"