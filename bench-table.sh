#!/usr/bin/env bash
# Koala vs LuaJIT vs Lua benchmark table (hyperfine mean, ms).
#
# usage: ./bench-table.sh [bench ...]      e.g. ./bench-table.sh fib tak
#   BUILD=build/debug ./bench-table.sh      use another koala build (default build/release)
#   RUNS=10 WARMUP=3 ./bench-table.sh       hyperfine runs / warmup
#
# koala runs with all the optimizations: inline (default), recursive-self-inline, domtree, unroll.
# The table is printed and written to bench-table.md.

set -e
cd "$(dirname "$0")"
source env.sh

BUILD=${BUILD:-build/release}
RUNS=${RUNS:-10}
WARMUP=${WARMUP:-3}
KOALA_OPTS=${KOALA_OPTS:-"--recursive-self-inline --domtree --unroll"}

# koala spawns koalac from PATH: use the koalac of the same build
export PATH="$PWD/$BUILD/bin:$PATH"
[ "$(command -v koalac)" = "$PWD/$BUILD/bin/koalac" ] || { echo "koalac not found in $BUILD/bin" >&2; exit 1; }

BENCHES=("$@")
[ ${#BENCHES[@]} -gt 0 ] || BENCHES=(tak loop-sum test_float_arith fib test_logic_chain test_float_logic sum ack shuffle)

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# mean in ms of a command
measure() {
    hyperfine --warmup "$WARMUP" --min-runs "$RUNS" -N --style none \
        --export-json "$tmp/r.json" "$1" >/dev/null 2>&1 || { echo "-"; return; }
    python3 -c "import json;print(round(json.load(open('$tmp/r.json'))['results'][0]['mean']*1000))"
}

# the optimized koala (JIT and interpreter) prints the same as the koala without the optimizations
check() {
    local b=$1 ref out
    ref=$(KOALA_JIT=0 koala "bench/$b.kl" 2>&1)
    for j in 1 0; do
        out=$(KOALA_JIT=$j koala "bench/$b.kl" ++ $KOALA_OPTS 2>&1)
        [ "$out" = "$ref" ] || echo "warning: $b KOALA_JIT=$j ++ $KOALA_OPTS prints '$out', expected '$ref'" >&2
    done
}

out=bench-table.md
{
    echo "koala: $BUILD, ++ $KOALA_OPTS; mean of >= $RUNS runs (ms)"
    echo
    echo "| bench | koala JIT | koala 解释 | luajit JIT | luajit 解释 | lua5.4 | koala JIT 加速 | vs luajit JIT |"
    echo "|---|---|---|---|---|---|---|---|"
} | tee "$out"

for b in "${BENCHES[@]}"; do
    check "$b"
    kj=$(KOALA_JIT=1 measure "koala bench/$b.kl ++ $KOALA_OPTS")
    ki=$(KOALA_JIT=0 measure "koala bench/$b.kl ++ $KOALA_OPTS")
    lj=$(measure "luajit bench/$b.lua")
    li=$(measure "luajit -joff bench/$b.lua")
    l5=$(measure "lua5.4 bench/$b.lua")
    python3 - "$b" "$kj" "$ki" "$lj" "$li" "$l5" <<'EOF' | tee -a "$out"
import sys
b, kj, ki, lj, li, l5 = sys.argv[1:]
def f(x): return float(x) if x != '-' else None
KJ, KI, LJ = f(kj), f(ki), f(lj)
speed = f"{KI / KJ:.1f}×" if KJ and KI else '-'
if not KJ or not LJ: vs = '-'
elif KJ <= LJ: vs = f"koala 胜 {LJ / KJ:.2f}× 🏆"
else: vs = f"luajit {KJ / LJ:.2f}×"
print(f"| {b} | {kj} | {ki} | {lj} | {li} | {l5} | {speed} | {vs} |")
EOF
done
