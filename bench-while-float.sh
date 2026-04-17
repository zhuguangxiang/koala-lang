#!/usr/bin/env bash

run() {
    name=$1
    cmd=$2
    echo "=== $name ==="
    hyperfine --warmup 3 --min-runs 10 --shell=none "$cmd" | tee -a results-while-float.txt
    echo
}

echo "Koala vs Lua vs luajit Benchmark Results" > results-while-float.txt
echo "==============================" >> results-while-float.txt

run "Koala test_float_logic"  "koala bench/test_float_logic.kl"
run "Lua test_float_logic"    "lua bench/test_float_logic.lua"
run "luajit test_float_logic" "luajit -joff bench/test_float_logic.lua"

run "Koala test_float_arith"  "koala bench/test_float_arith.kl"
run "Lua test_float_arith"    "lua bench/test_float_arith.lua"
run "luajit test_float_arith" "luajit -joff bench/test_float_arith.lua"


echo "Done."
