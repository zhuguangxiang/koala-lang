# 19. dict 常量键应在编译期合并（逻辑已写好但整条链不可达）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 算法已存在但零调用者，接入点待定；依赖 [#18](issue-18.md)

---

### 详细描述

<details>
<summary><b>🎯 需求</b></summary>

dict 字面量中重复的**常量**键应在编译期就合并掉，少发 `seq.set`，而不是全部留给运行期 dict 覆盖。

</details>

<details>
<summary><b>🔍 现状</b></summary>

编译期**完全不合并**。Phase 3 后 `{1:"a", 2:"b", 1:"c"}` 发出 **3 条 `seq.set`**（重复的 `1` 出现两次）；改动前的 `build_intern @map, 1,'a', 2,'b', 1,'c'` 同样是 6 个操作数、不去重。

即此行为**改动前后一致，不是本轮引入的回退**。

</details>

<details>
<summary><b>📦 已有实现（全仓零调用者）</b></summary>

`src/parser/klr/ir.c` 里整套编译期合并逻辑**均已写好**，但 `klr_const_map` **全仓零调用者**：

- **`klr_const_map`**（ir.c:339）只做两件事 —— 调 `klr_const_merged_list`，再把 `which` 从 `CONST_LIST` 改标为 `CONST_MAP`。
- **`klr_const_merged_list`**（ir.c:297）是真正的算法：按 `key0,val0,key1,val1,...` 遍历，`out` 为写游标**原地压缩**；命中重复键时只覆盖 `items[2*dup+1] = val` 并 `continue`（`out` 不前进），即**首次出现占位、最后一个值生效**，与运行期覆盖语义一致。
- **`is_mergeable_key`**（ir.c:233）白名单：常量且 `which ∈ {INT, UINT, FLT, BOOL, STR, TUPLE, RANGE}`；排除 list/map，因其可变、运行期可能按身份而非内容比较。
- **`mergeable_key_equal`**（ir.c:255）：要求两者 `which` 相同；INT/UINT 比 `len`+`ival`；FLT 直接用 `==` 且**故意忽略位宽**（`0.0 == -0.0` 合并、`NaN` 永不合并，对齐运行期 `_dict_entry_eq_()`）；STR 先比 intern 指针再回退 `memcmp`；TUPLE/RANGE 递归逐元素，任一元素不可比则整体判不等。设计上**保守** —— 判不等只会少合并，不会误合并。

</details>

<details>
<summary><b>🚧 不可达原因</b></summary>

- `const_copy_prop.c:1264` 的常量 call 折叠分支只处理 `list` / `tuple` / `int64`，**不走 map**。
- irgen 的 `emit_ir_map` 现在直接发 `new` + `seq.set`，**也不经过常量池**。

</details>

<details>
<summary><b>✅ 语义已核对</b></summary>

`{1:"a", 1.0:"b"}` 运行期输出 `{1: a, 1: b}`（**不合并**），说明「`which` 必须相同」这条前置检查与运行期一致 —— **koala dict 不像 Python 那样跨类型合并键**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办与约束</b></summary>

决定接入点 —— 在 `emit_ir_map` 里对常量键先去重再发 `seq.set`，或复用/改造 `klr_const_merged_list`。

约束：

1. **只能合并常量键**；对象键（自定义 `__hash__`/`__eq__`）必须保留逐条 `seq.set`。
2. 合并**不得吞掉用户 `hash()` 抛出的异常**，也不得改变条目求值顺序带来的副作用。
3. 需**先解决 [#18](issue-18.md) 的 bool/int 键语义**，否则编译期合并规则无法与运行期对齐。

</details>

<details>
<summary><b>🔗 关联：需撤销一处删除决定</b></summary>

`docs/Koala_TODO.md` §item 3 原计划把 `klr_const_map` / `klr_const_merged_list` / `is_mergeable_key` / `mergeable_key_equal` 当**死代码删除**（§27）。

本条要求改为**保留并接上**，该删除决定需相应撤销。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 记录，用户要求后续解决。

</details>