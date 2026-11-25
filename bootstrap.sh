#!/bin/bash
# Build `libs/std` with the `koala` in PATH, then run its tests.
#
# `std/builtin` is the language core: it is compiled by `koalac` with
# `--build-stdlib`, because it must not import `std/builtin` itself. Every
# other package is a plain `koala -c`. The order below follows `dependencies`
# in each `__pkg__.toml` (`std/wombat` comes after `std/toml`).

set -e

cd "$(pwd)/libs"

mkdir -p std/dist/std

koalac --build-stdlib --cgen --write-klc --fusion -o std/dist/std/builtin.klc --package-name=std/builtin std/builtin
koala -c -o std/dist/std/io.klc --package-name=std/io std/io
koala -c -o std/dist/std/os.klc --package-name=std/os std/os
koala -c -o std/dist/std/fs.klc --package-name=std/fs std/fs
koala -c -o std/dist/std/sys.klc --package-name=std/sys std/sys
koala -c -o std/dist/std/prelude.klc --package-name=std/prelude std/prelude
koala -c -o std/dist/std/collections.klc --package-name=std/collections std/collections
koala -c -o std/dist/std/gc.klc --package-name=std/gc std/gc
koala -c -o std/dist/std/time.klc --package-name=std/time std/time
koala -c -o std/dist/std/ut.klc --package-name=std/ut std/ut
koala -c -o std/dist/std/socket.klc --package-name=std/socket std/socket
koala -c -o std/dist/std/math.klc --package-name=std/math std/math

for t in std/*/test/*.kl; do
    koala --test "$t"
    KOALA_JIT=0 koala --test "$t"
done