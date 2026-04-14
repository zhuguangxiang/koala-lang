#!/usr/bin/env bash

run() {
    name=$1
    cmd=$2
    echo "=== $name ==="
    hyperfine --warmup 3 --min-runs 10 --shell=none "$cmd" | tee -a results-kl.txt
    echo
}

echo "Koala Benchmark Results" > results-kl.txt
echo "==============================" >> results-kl.txt

run "Koala sum"  "koala bench/sum.kl"
run "Koala loop-sum"  "koala bench/loop-sum.kl"
run "Koala fib"  "koala bench/fib.kl"
run "Koala ack"  "koala bench/ack.kl"
run "Koala tak"  "koala bench/tak.kl"
run "Koala shuffle"  "koala bench/shuffle.kl"

echo "Done."
