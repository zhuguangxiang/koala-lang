# 17. `libs/std/dist/*.klc` 陈旧产物导致运行时 "func not implemented"

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 已用重新构建绕过，ABI 指纹/连带重建均未实施

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

改完编译器只跑 `./build-debug.sh`，`test-ut` 有 **11 个用例**报 `Error: func 'tuple::__init__' is not implemented!`。

原因：`libs/std/dist/*.klc` 还是**上一版编译器**（tuple 也走 `new`+`__init__` 时）编出来的，里面残留 `call @tuple:__init__`，而新编译器已不再提供该原生函数。

</details>

<details>
<summary><b>🧪 复现</b></summary>

在 list/tuple 构造路径来回改动后，只 `./build-debug.sh` 不重新 `wombat build`，再跑 `./test-debug.sh`。

</details>

<details>
<summary><b>🩹 绕过办法</b></summary>

```
cd libs/std && wombat build
```

然后 `./build-debug.sh` 重新安装。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

`.klc` 里记录**编译器版本 / ABI 指纹**，加载时不匹配就报错或自动重编。

至少让 `build-debug.sh` 在编译器源码变动后**连带重建标准库产物**，避免这类「编译通过、运行才炸」的静默不一致。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 发现于 Phase 2 回退 tuple 期间，已用重新构建绕过。

</details>