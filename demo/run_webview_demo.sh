#!/bin/bash
#
# run_webview_demo.sh — 运行 koala/webview 桌面 app 示例。
#
# 在任意目录执行均可：脚本会先切到仓库根目录，再设置库/包搜索路径。
# 需先构建 koala/webview（wombat build libs/koala/webview）。
#
#     ./demo/run_webview_demo.sh
#
# 关闭窗口即退出。

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

export LD_LIBRARY_PATH=libs/koala/dist/koala:$HOME/.local/lib:$LD_LIBRARY_PATH
export KOALA_PATH=libs/koala/dist:libs/std/dist

exec koala demo/webview_demo.kl