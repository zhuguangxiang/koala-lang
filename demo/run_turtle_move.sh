#!/bin/bash
#
# run_turtle_move.sh — 运行 koala/turtle 交互示例：方向键移动小球。
#
# 在任意目录执行均可。需先构建 koala/turtle 和安装 SDL2。
#
#     ./demo/run_turtle_move.sh
#
# 操作：↑ ↓ ← → 移动；关闭窗口或按 Esc 退出。

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

export LD_LIBRARY_PATH=libs/koala/dist/koala:$HOME/.local/lib:$LD_LIBRARY_PATH
export KOALA_PATH=libs/koala/dist:libs/std/dist

exec koala demo/turtle_move.kl