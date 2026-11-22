#!/bin/bash
#
# run_plot_demo.sh — 运行 koala/plot 功能演示菜单（plot_demo.kl）。
#
# 在任意目录执行均可：脚本会先切到仓库根目录，再设置库/包搜索路径。
# 需先构建 koala/plot（wombat build plot，会带上 koala/webview、koala/json、
# koala/math、koala/readline）。
#
#     ./demo/run_plot_demo.sh
#
# 终端菜单 1-4 一页一图，5 是 2x2 多图一页；关窗回到菜单，0 退出。
# 无显示环境可改用 html 后端（每个菜单项写出 plot.html，覆盖式）：
#
#     KOALA_PLOT_BACKEND=html ./demo/run_plot_demo.sh
#     printf '5\n0\n' | KOALA_PLOT_BACKEND=html ./demo/run_plot_demo.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

export LD_LIBRARY_PATH=libs/koala/dist/koala:$HOME/.local/lib:$LD_LIBRARY_PATH
export KOALA_PATH=libs/koala/dist:libs/std/dist

exec koala demo/plot_demo.kl