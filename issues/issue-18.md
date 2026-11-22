# 18. dict 字面量混用 bool 键与 int 键时断言崩溃

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— SIGABRT，且未定性是否本轮引入

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`{true: "a", 1: "b"}` 触发：

```
object.h:519: to_raw_bool: Assertion '((v)->tag > (64 + 1))' failed
```

进程 SIGABRT（**退出码 134**）。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

```
func main() { print({true: "a", 1: "b"}) }
```

</details>

<details>
<summary><b>📊 边界对照</b></summary>

同一文件里：

| 字面量 | 结果 |
|---|---|
| `{true: "a", 1: "b"}` | ❌ SIGABRT |
| `{1: "a", 1.0: "b"}` | ✅ 输出 `{1: a, 1: b}` |
| `{0.0: "a", -0.0: "b"}` | ✅ 输出 `{0: b}` |

</details>

<details>
<summary><b>🧠 初步分析</b></summary>

bool 与 int 的**哈希值相同**，两个键落进同一比较分支后，dict 的 key 相等性判断把 int 槽当 bool 解包，`to_raw_bool` 收到 int tag 而断言失败。

疑为**类型判别缺失**，而非断言本身过严。

</details>

<details>
<summary><b>❓ 未定性</b></summary>

尚未判定是本轮 dict lowering 改动（`new` + `__init__` + 逐条 `seq.set`）引入，还是既有缺陷。

需用**隔离 `PATH` + `KOALA_PATH`** 的改动前 release 二进制跑同一探针对比才能定性（驱动经 PATH 解析 `koalac`，混用不同 build 会静默串味，见 [#21](issue-21.md)）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

定位 `to_raw_bool` 在 dict key eq/hash 路径上的调用链，在比较前**补齐键类型校验**。

⚠️ **不要通过放宽断言掩盖。**

</details>

<details>
<summary><b>🧪 建议测试</b></summary>

`test/test-run/test_dict_bool_int_key.kl`，覆盖 `{true: 1}`、`{1: true}`、`{true: "a", 1: "b"}`、`{false: "a", 0: "b"}`，锁定预期输出与是否合并。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 发现于 Phase 3 收尾核对 `klr_const_map` 语义期间，用户确认先记录、待后续处理。

</details>