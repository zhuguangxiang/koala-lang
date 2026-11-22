#!/usr/bin/env bash

run() {
    name=$1
    cmd=$2
    echo "=== $name ==="
    hyperfine --warmup 3 --min-runs 10 --shell=none "$cmd" | tee -a results-while-int.txt
    echo
}

echo "Koala vs Lua vs luajit Benchmark Results" > results-while-int.txt
echo "==============================" >> results-while-int.txt

run "Koala test_logic_chain"  "koala bench/test_logic_chain.kl"
run "Lua test_logic_chain"    "lua bench/test_logic_chain.lua"
run "luajit test_logic_chain" "luajit -joff bench/test_logic_chain.lua"

run "Koala test_arith"  "koala bench/test_arith.kl"
run "Lua test_arith"    "lua bench/test_arith.lua"

run "Koala test_bitwise"  "koala bench/test_bitwise.kl"
run "Lua test_bitwise"    "lua bench/test_bitwise.lua"

run "Koala test_uint_arith"  "koala bench/test_uint_arith.kl"

echo "Done."
