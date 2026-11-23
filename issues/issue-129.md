# 工具链静默失效：KOALA_PATH 层级错误回落安装版、wombat 吞诊断、安装驱动器缺 rpath

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 3 项均于 2026-11-22 以 koala 0.459（build 505）复现；症状均为「静默」或「缺诊断」，易被误判为编译器 bug

> 来源：本次 plot 回归与 issues 探针过程中亲历，均为「伪装成语言 bug 的工具链问题」

---

### 详细描述

<details>
<summary><b>1️⃣ KOALA_PATH 指错层级：import 静默回落到安装前缀的陈旧 .klc</b></summary>

KOALA_PATH 必须指向 `dist`（即 `koala/` 目录的**父目录**），指向 `dist/koala` 时 import 静默回落到已安装前缀 `/home/james/.local/lib/koala/lib/koala-lib` 里的陈旧 .klc。症状是「新改的库代码不生效」甚至「report func not implemented」，与编译器 bug 无异。用 gdb 追 `koalac` 的 `type_spec_from_str` 才发现回落路径。建议：层级错误时打警告或报错，或先查找 `$KOALA_PATH/koala/` 再回落。

</details>

<details>
<summary><b>2️⃣ `wombat build <pkg>` 只打印「koala: compilation failed」，无任何诊断</b></summary>

`wombat build json` 等只输出 `koala: compilation failed` 并 exit 255，不转发行号/错误信息。必须回到 `libs/` 手动重跑底层 koalac 命令才能看到真实错误。诊断被吞导致用户在正确命令（能看到错误）与 wombat 之间反复试错。

</details>

<details>
<summary><b>3️⃣ 已安装驱动器 `/home/james/.local/bin/koala` 缺 rpath：裸调用找不到 libkoala.so</b></summary>

```text
$ koala --version
koala: error while loading shared libraries: libkoala.so: cannot open shared object file
```

带 `LD_LIBRARY_PATH=dist/koala:$HOME/.local/lib` 才正常。安装脚本未给驱动器写入 rpath，或者安装说明未要求把 lib 目录写进 ld.so.conf。裸 `koala` 对新用户不可用，与 [#21](issue-21.md)（PATH 遮蔽）同属「安装态 vs 构建态」问题。

</details>

---

### 修复方案

<details>
<summary><b>🛠 方向</b></summary>

- 第 1 项：KOALA_PATH 层级校验（目录里必须有 `koala/` 子目录），失败时警告或报错；或在回落安装前缀前显式告警。
- 第 2 项：wombat build 转发布 koalac 的完整 stderr 与退出码。
- 第 3 项：安装脚本给驱动器写 rpath（`-Wl,-rpath,$ORIGIN/../lib`）或安装时记录 `LD_LIBRARY_PATH` 提示。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#21](issue-21.md)：驱动器经 PATH 找 koalac，旧安装版静默遮蔽新构建（同族安装态问题）。
- [#17](issue-17.md)：陈旧 .klc 产物导致 func not implemented（第 1 项的回落目标正是陈旧产物）。

</details>
