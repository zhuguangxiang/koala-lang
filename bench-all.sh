#!/usr/bin/env bash

run() {
    name=$1
    cmd=$2
    echo "=== $name ==="
    hyperfine --warmup 3 "$cmd" | tee -a results.txt
    echo
}

echo "Koala vs Lua vs luajit Benchmark Results" > results.txt
echo "==============================" >> results.txt

run "Koala sum"  "koala bench/sum.kl"
run "Lua sum"    "lua bench/sum.lua"
run "luajit sum" "luajit -joff bench/sum.lua"

run "Koala fib"  "koala bench/fib.kl"
run "Lua fib"    "lua bench/fib.lua"
run "luajit fib" "luajit -joff bench/fib.lua"

run "Koala ack"  "koala bench/ack.kl"
run "Lua ack"    "lua bench/ack.lua"
run "luajit ack" "luajit -joff bench/ack.lua"

echo "Done."
