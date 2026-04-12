#!/usr/bin/env bash

./build-release.sh

run() {
    name=$1
    cmd=$2
    echo "=== $name ==="
    hyperfine --warmup 3 --min-runs 10 --shell=none "$cmd" | tee -a results.txt
    echo
}

echo "Koala vs Lua vs luajit Benchmark Results" > results.txt
echo "==============================" >> results.txt

run "Koala sum"  "koala bench/sum.kl"
run "Lua sum"    "lua bench/sum.lua"
run "luajit sum" "luajit -joff bench/sum.lua"

run "Koala loop-sum"  "koala bench/loop-sum.kl"
run "Lua loop-sum"    "lua bench/loop-sum.lua"
run "luajit loop-sum" "luajit -joff bench/loop-sum.lua"
# run "js loop-sum"     "node --jitless bench/loop-sum.js"

run "Koala fib"  "koala bench/fib.kl"
run "Lua fib"    "lua bench/fib.lua"
run "luajit fib" "luajit -joff bench/fib.lua"
# run "js fib"     "node --jitless bench/fib.js"

run "Koala ack"  "koala bench/ack.kl"
run "Lua ack"    "lua bench/ack.lua"
run "luajit ack" "luajit -joff bench/ack.lua"
# run "js ack"     "node --jitless bench/ack.js"

run "Koala tak"  "koala bench/tak.kl"
run "Lua tak"    "lua bench/tak.lua"
run "luajit tak" "luajit -joff bench/tak.lua"
# run "js tak"     "node --jitless bench/tak.js"

run "Koala shuffle"  "koala bench/shuffle.kl"
run "Lua shuffle"    "lua bench/shuffle.lua"
run "luajit shuffle" "luajit -joff bench/shuffle.lua"
# run "js shuffle"     "node --jitless bench/shuffle.js"

echo "Done."
