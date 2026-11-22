#!/bin/bash
#
# run_plot_demo.sh — 运行 koala/plot 绘图示例（ML 主题 2x2 图）。
#
# 在任意目录执行均可：脚本会先切到仓库根目录，再设置库/包搜索路径。
# 需先构建 koala/plot（wombat build plot，会带上 koala/webview、koala/json、
# koala/math）。
#
#     ./demo/run_plot_demo.sh
#
# 关闭窗口即退出。无显示环境可改用 html 后端（写出 plot.html）：
#
#     KOALA_PLOT_BACKEND=html ./demo/run_plot_demo.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

export LD_LIBRARY_PATH=libs/koala/dist/koala:$HOME/.local/lib:$LD_LIBRARY_PATH
export KOALA_PATH=libs/koala/dist:libs/std/dist

exec koala demo/plot_demo.kl