# enum 后续 review 场景（待检查）

← 返回 [问题列表](issues.summary.md)

- [ ] 待检查 —— 4 个场景待补 review / 回归验证（**不是已确认缺陷**）

> 来源：`docs/Koala_TODO.md` §88（2026-11-10，待检查）

---

### 详细描述

<details>
<summary><b>📋 待补场景</b></summary>

- [ ] **跨包同名 enum**：两个包分别定义同名公开 enum，确认赋值、比较和泛型实例保持类型隔离；泛型函数名 mangling 不因 enum 同名而冲突。
- [ ] **enum 作为 dict key**：检查 hash、相等性、插入、覆盖、查询和删除的一致性，并确认语义阶段保留 enum 身份，IR 阶段按 int64 表示。
- [ ] **KLC int64 边界值往返**：公开 enum 成员包含 int64 最小值、最大值和负数，写入 KLC 后由另一包读取，确认成员值完整保留。
- [ ] **类型与值的错误路径**：enum 类型名允许作为泛型类型实参，但裸 enum 不能作为值，enum 成员（如 `A.X`）也不能作为类型实参；覆盖本地与包限定写法。

</details>

---

### 修复方案

<details>
<summary><b>📌 范围说明</b></summary>

以上是**待补充 review / 回归验证**的场景，不是已确认缺陷。发现缺陷后按实测结果单独立条目。

`match` 与 display / `fmt` 属于**暂缓讨论**的功能，不纳入当前 enum 支持的完成范围。

</details>