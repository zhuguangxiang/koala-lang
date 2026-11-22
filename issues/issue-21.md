# 21. koala 驱动器经 PATH 找 koalac，旧安装版会静默遮蔽新构建

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 已用 install-debug.sh 绕过，驱动器解析逻辑未改

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`./build-debug.sh` 后直接用 `./build/debug/bin/koala x.kl` 验证，执行的仍是 `~/.local/bin/koalac`（**旧安装版**）：驱动器用 `posix_spawnp("koalac", ...)` 按 PATH 解析编译器，而 `env.sh` 并未把 `build/debug/bin` 加入 PATH。

本轮 SSA 修复后曾**误判「修复无效」**，实为旧编译器在跑。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

改动编译器后 `./build-debug.sh`（不 install），`source env.sh && ./build/debug/bin/koala x.kl`；`which koalac` 指向 `~/.local/bin/koalac`。

</details>

<details>
<summary><b>💥 影响</b></summary>

所有「构建后直接跑 `build/debug/bin/koala`」的验证**都不可信**。

lit **不受影响**（`lit.cfg` 自行把构建目录置顶 PATH）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

驱动器**优先按自身可执行文件所在目录**解析 koalac（`/proc/self/exe` 或 `argv[0]` dirname），PATH 仅作回退。

或启动时比对版本/构建时间并在不一致时告警。

</details>

<details>
<summary><b>🩹 短期约定</b></summary>

验证前 `export PATH=$(pwd)/build/debug/bin:$PATH`，或先 `./install-debug.sh`。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 记录于 lambda/SSA 修复期间，已用 install-debug.sh 绕过，待讨论。

</details>