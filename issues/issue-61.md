# datetime 固定偏移时区（待设计）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 唯一未落地的设计缺口，两个设计点待作者拍板

> 来源：`docs/Koala_TODO.md` §23（待设计）

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`libs/koala/datetime` 的 `Date` / `Time` / `DateTime` 三类已收工并全绿（构造校验、`to_str`/`fmt`、`from_str`、比较、`hash`、`+`/`-` 与 `diff`、`Date.today`、`DateTime.combine`/`replace`、`timestamp` 家族、`now`/`to_instant`/`from_instant`）。

**唯一未落地的设计缺口是固定偏移时区**（作者 2026-10 决定排到最后，届时单独拍板）。

当前 `DateTime` 隐含 UTC，无偏移字段、不输出 `+08:00`/`Z`、无从偏移串解析、无 `astimezone()`。现状仅靠 `to_instant`/`from_instant` 与 `std/time` 的 realtime(UTC) 时钟桥接。

</details>

<details>
<summary><b>🔒 约束前提</b></summary>

- Koala `Instant`/`sys_clock` 全走 UTC，**无本地时区探测、无命名时区数据库**（`ZoneInfo`/tzdata/DST 出局，已定）。
- 因此只能做「UTC + 显式固定偏移」，对齐 Python `timezone(timedelta(...))`，非 `tzinfo` 全功能。
- 纯 Koala 实现，零新增 C native（沿用「能用 koala 实现就用 koala」约束）。

</details>

---

### 修复方案

<details>
<summary><b>❓ 两个待拍设计点（作者开工前定）</b></summary>

1. **类型形态**：
   - **A** 新建独立 `Timezone` 类存固定偏移（对齐 Python `tzinfo`）；`DateTime` 加可选 `tz Timezone?` 字段，`nil` = naive。可区分 naive vs UTC-aware。
   - **B** 直接在 `DateTime` 加 `offset_seconds int`（0=UTC）。更轻，但**无法区分 naive 与 UTC-aware**。
2. **本轮范围**：
   - **小步**：先做建模 + `to_str`/`from_str` 读写偏移（`+08:00`/`Z`）+ 构造带 tz；比较 / `astimezone` 留下一轮。
   - **大步**：一次做完 aware/naive 比较规则（跨时区按绝对时刻、naive 不与 aware 比）+ `astimezone()` + 全部测试。

</details>

<details>
<summary><b>📝 备注</b></summary>

- 落 `from_str` 通用 ISO（含 `Z`/`±HH:MM`）与时区解析应与此块合并考虑（当前 `from_str` 定案为**严格模式，仅逆向 `to_str`**）。
- `Timezone` 类型若新增须遵循包内命名纪律（类名不重复 `datetime` 前缀）与 `pub let` 只读字段约定。

</details>