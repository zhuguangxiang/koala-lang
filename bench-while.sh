#!/usr/bin/env bash

run() {
    name=$1
    cmd=$2
    echo "=== $name ==="
    hyperfine --warmup 3 --min-runs 10 --shell=none "$cmd" | tee -a results-while.txt
    echo
}

echo "Koala vs Lua vs luajit Benchmark Results" > results-while.txt
echo "==============================" >> results-while.txt

run "Koala test_logic_chain"  "koala bench/test_logic_chain.kl"
run "Lua test_logic_chain"    "lua bench/test_logic_chain.lua"
run "luajit test_logic_chain" "luajit -joff bench/test_logic_chain.lua"

run "Koala test_arith"  "koala bench/test_arith.kl"
run "Lua test_arith"    "lua bench/test_arith.lua"

run "Koala test_bitwise"  "koala bench/test_bitwise.kl"
run "Lua test_bitwise"    "lua bench/test_bitwise.lua"

echo "Done."
