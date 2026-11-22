# 声明-实现欠账清单（`@native` 无 C 实现 / 类型未注册）

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— 5 个 collections 模块已四项对齐；7 个模块 85 个 `@native` 零实现；`os` 全缺

> 来源：`docs/Koala_TODO.md` §16（2026-10-02 建立）
>
> libs/std 全面盘点（40 个 `.kl` 全文 × `src/` 全文 grep 交叉核实）：以下声明已进入 klc 构建，但运行时侧无实现 —— 调用将命中 `not_impl` 哨兵（§7.1：加载期警告 + 调用时 `Exception("... is not implemented.")`）。设计文档只记设计与声明，**欠账在此挂账**。

---

### 详细描述

<details>
<summary><b>🗂 容器与类型（16.1）</b></summary>

- `dict`：25 个方法全部 `@native`；`src/` 无 dict 类型实现与注册，且当前 196 个测试中**没有任何 dict 用例**。
- `hashset` / `TreeSet`：19 / 20 个方法全部 `@native`；无 C 实现、无测试。
- `koala.y` 仍留 `set_type` 文法残段（行为行已注释），集合字面量语法未启用。

</details>

<details>
<summary><b>✅ collections 已四项对齐（16.1b，2026-10-24 全量核对）</b></summary>

| 模块 | 类型 | @native | C 方法表 | 注册 | 测试 |
|---|---|---|---|---|---|
| `bitset.kl` | `BitSet` | 15 | 15 | ✅ | `test_bitset.kl`（28） |
| `stack.kl` | `Stack[T]` | 8 | 8 | ✅ | `test_stack.kl`（28） |
| `queue.kl` | `Queue[T]` | 8 | 8 | ✅ | `test_queue.kl`（34） |
| `linkedlist.kl` | `LinkedList[T]` + 2 迭代器 | 29 | 29 | ✅ | `test_linkedlist.kl`（50） |
| `deque.kl` | `Deque[T]` + 2 迭代器 | 29 | 29 | ✅ | `test_deque.kl`（67） |

合计 **207 用例全通过**；`valgrind -q` 下 `test_deque` 零错误；`clang-format` 与编译均无告警。

<details>
<summary><b>❌ 7 个模块 / 85 个 `@native` 方法零实现（C 侧零实现、零注册、零测试）</b></summary>

| 模块 | 类型（trait） | @native | 实测症状 |
|---|---|---|---|
| `frozenbytes.kl` | `FrozenBytes : Sequence[uint8]` | 8 | 运行期 `Error: func 'FrozenBytes::__init__' is not implemented!` |
| `frozendict.kl` | `FrozenDict[K, V] : Map[K, V]` | 11 | 运行期同上（`FrozenDict::__init__`） |
| `sorteddict.kl` | `SortedDict[K : Comparable, V] : MutableMap[K, V]` | 19 | 运行期同上（`SortedDict::__init__`） |
| `frozenlist.kl` | `FrozenList[T] : Sequence[T]` | 8 | **编译期崩溃**：`symbol.c:699: Why goes here?` |
| `frozenset.kl` | `FrozenSet[T] : Set[T]` | 11 | 编译期崩溃，同上 |
| `heapq.kl` | `Heap[T : Comparable]` | 8 | 编译期崩溃，同上 |
| `sortedset.kl` | `SortedSet[T : Comparable] : MutableSet[T]` | 20 | 编译期崩溃，同上 |

</details>

<details>
<summary><b>🐞 顺带发现的编译器缺陷：同一状态两种报错方式</b></summary>

实例化「全部方法 `@native` 且 C 侧无实现」的类时，报错方式随**类型参数个数**而变：

- 单类型参数的（`Heap` / `SortedSet` / `FrozenList` / `FrozenSet`）在 `src/parser/symbol.c` 的 `UNREACHABLE()` 处崩溃（打印 `Why goes here?` 后 `compilation failed`，**无任何诊断信息**）。
- 零参数的 `FrozenBytes` 与双类型参数的 `FrozenDict` / `SortedDict` 能正常编译到运行期，调用时命中 `not_impl` 哨兵（§7.1 的预期路径）。

即「缺实现」这一种状态有两种报错方式，**单参数泛型那条缺诊断**，根因待查（疑在 lro / 接口索引计算）。

</details>
<details>
<summary><b>📦 fs / os / 数值方法（16.2）</b></summary>

- **`fs`**：C 侧**全部已实现** —— `open` / `read_bytes` / `write_bytes` / `write_str` / `exists` / `is_file` / `is_dir` / `size` / `remove` / `mkdir` / `rmdir` / `list_dir` 与 `File` 方法；`Path` 方法为纯 KL 实现。
- **`os`**：`system` / `run` + `Process` 8 方法 + `PipeReader` / `PipeWriter` 8 方法 —— **声明活跃、native 全缺**（仅 `getenv` / `setenv` / `unsetenv` 已实现）。
- 数值方法族：见 §13.6 新增条目。

</details>

<details>
<summary><b>🧹 命名 / 一致性 WIP（16.3，顺手项）</b></summary>

- `MutableSequence.append` 与 `list.append` 已统一。
- `Buffer`（bytebuf.kl，原 `ByteBuf`）已收缩为纯 append 式缓冲、不再声明 `MutableSequence[uint8]`；设计文档 §8.4 已同步。
- `seeker.kl` 文档提及 `io.StdinReader` —— **不存在的类**（doc-only 残留，示例应改用 `sys.stdin`）。
- `ut.assert_eq[T : Equatable]`（裸约束）与 `assert_ne[T : Equatable[T]]`（完整形参）约束写法不一致，可顺手统一。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议实现顺序（性价比从高到低）</b></summary>

1. **`heapq`**：8 个方法、无索引访问语义，可独立实现二叉堆；**顺带修掉上面那个单参数泛型的编译期崩溃**。
2. **`frozenbytes` / `frozenlist` / `frozenset` / `frozendict`**：只读视图，方法少；但 `FrozenDict` 依赖 dict 本体（16.1 仍是欠账），`FrozenSet` 同理依赖 set。
3. **`sorteddict` / `sortedset`**：39 个方法，需要平衡树或跳表，并依赖 `Comparable` 的跨类型比较分发。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- §7.1（`not_impl` 哨兵的加载期警告与调用时异常）。
- [#107](issue-107.md)（§28 `os.run`）：`os` 的 native 缺口与 finalizer 接线问题同源。
- [#97](issue-97.md)（§79）：`koala/*` 的 native handle 也缺 `_free`。

</details>
</details>