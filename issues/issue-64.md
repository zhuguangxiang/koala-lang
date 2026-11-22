# 添加 fs / os API 时发现的编译器与工具问题（7 项）

← 返回 [问题列表](issues.summary.md)

- [ ] 部分修复（1/7）：`list.sort()` 已实现；余 6 项未处理

> 来源：`docs/Koala_TODO.md` §38（2026-11-01，to discuss）

---

### 详细描述

<details>
<summary><b>1️⃣ `in` 的优先级低于 `||` / `&&`</b></summary>

`koala.y` 中 `in_expr` 是 `expr` 顶层的 `or_expr IN or_expr`，所以 `a || "/" in s` 解析成 `(a || "/") in s` 并类型检查失败。

Python 把 `in` 放在比较层，高于 `and` / `or`。`std/fs/path.kl`（`Path.with_name`）目前用括号绕过。

</details>

<details>
<summary><b>2️⃣ 编译错误没有诊断信息</b></summary>

方法里的 `if new_name.empty() || "/" in new_name || "\\" in new_name { ... }` 只打印 "koala: compilation failed"；更短的 `x.empty() || "/" in s` 会报 "argument type mismatch for operator '||'"。

说明某条错误路径**没有调用 `kl_error`**。

</details>

<details>
<summary><b>3️⃣ 关键字被用作名字</b></summary>

`let link = "..."` 只报 "expected an identifier."（外加级联错误）；应当说明 `link` 是关键字。

</details>

<details>
<summary><b>4️⃣ `tools/kl-doc.py` 锚点冲突</b></summary>

`Stat` 类与 `stat` 函数在 `docs/std_fs_api.md` 里都得到锚点 `#stat`（锚点是小写化名字），页内链接互相冲突。

</details>

<details>
<summary><b>5️⃣ SIGPIPE 在两处被忽略</b></summary>

`os_native_lib_init` 与 `socket_native_lib_init` 都调用 `signal(SIGPIPE, SIG_IGN)`；无害，但可以合并成运行期启动时的一次调用。`os.run` / `os.system` 的子进程会恢复默认行为（`init_spawn_attr`，§28）。

</details>

<details>
<summary><b>6️⃣ §25.4 内容过时</b></summary>

§25.4 说网络领域是空的，但 `std/socket` 已存在。

</details>

<details>
<summary><b>7️⃣ ✅ `list.sort()` 已修复（2026-11-08）</b></summary>

曾以 "func 'list::sort' is not implemented!" panic（2026-11-01 在 `fs.glob` 中发现）。运行时现在有 `_list_sort`（`src/objects/listobj.c`）：对 list 的 `TValue` 做稳定的自底向上归并排序，签名 `sort(reverse = false)`，排序通过 `SLOT_LT`（`__lt__`），因此用户类可排序。

`fs.glob` 本身仍按 `walk` 顺序返回 `**` 匹配（现已作为其契约记录，不再是缺 `sort` 的症状）；需要严格顺序的调用方对结果调 `sort()`。

测试：`test/test-run/test_list_sort.kl`、`test/test-ut/test_list_sort.kl`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待处理</b></summary>

- 第 1 项：把 `in` 提到比较层（`koala.y` 文法调整），并去掉 `std/fs/path.kl` 的括号绕过。
- 第 2 项：找出未调用 `kl_error` 的错误路径并补齐诊断。
- 第 3 项：标识符解析失败时，若名字命中关键字表，给出「`X` 是关键字」。
- 第 4 项：`tools/kl-doc.py` 锚点去重（类 / 函数加前缀或后缀）。
- 第 5 项：合并为运行期启动时一次 `signal(SIGPIPE, SIG_IGN)`。
- 第 6 项：更新 §25.4 的网络领域描述。

</details>

<details>
<summary><b>📌 开列（`list.sort` 之后仍缺）</b></summary>

- `sort_by(cmp)`
- `key=`

</details>