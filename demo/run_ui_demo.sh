#!/bin/bash
#
# run_ui_demo.sh — 运行 koala/ui 桌面 app 示例（零 HTML/JS/CSS）。
#
# 在任意目录执行均可：脚本会先切到仓库根目录，再设置库/包搜索路径。
# 需先构建 koala/ui（wombat build ui，会带上 koala/webview、koala/json）
# 以及 koala/datetime（wombat build datetime）。
#
#     ./demo/run_ui_demo.sh
#
# 关闭窗口即退出。

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

export LD_LIBRARY_PATH=libs/koala/dist/koala:$HOME/.local/lib:$LD_LIBRARY_PATH
export KOALA_PATH=libs/koala/dist:libs/std/dist

exec koala demo/ui_demo.kl