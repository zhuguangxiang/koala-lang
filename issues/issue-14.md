# 14. 数字字面量后紧跟 `+` / `-`（无空格）词法报错

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 候选修法倾向 B；kparse 侧另有暂定策略

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`let x = 2+3` 报 `syntax error`，`let x = 2-3` 同样；加空格 `2 + 3` 正常输出 `5`。`*` 不受影响（`2*3` 正常）。

**不限于 list 字面量**：`(2+3)` 报 `expected ')'`，`[2+3, 4]` 报 `expected ',' or ']'`，报错位置落在**表达式起点**而非 `+` 处。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

```
func main() { let x = 2+3; print(x) }
```

写成 `2 + 3` 即通过。

补充形态（2026-11-22）：

```
func main() { let a = 5; print(a-1) }
```

→ `error: expected an expr list or ')'.`；`print(a - 1)` 正常。`x+2`、`arr[i-1]`、`f(n-1)` 同理。

</details>

<details>
<summary><b>🧠 根因（2026-11-22 更正）</b></summary>

**标识符后紧跟 `-`/`+` 数字同样中招**，且根因**不是**指数符号，而是 `koala.l` 的数字规则**自带 `[+-]?`**（`[+-]?[0-9]([0-9_]*[0-9])?` 等四条）。

flex 最长匹配把 `a-1` 切成 `ID(a)` + `INT_LITERAL(-1)`，parser 看到两个相邻操作数即报 `expected an expr list or ')'`。

> ⚠️ 原「初步分析」（2026-11-21）认为是「数字扫描把紧跟的 `+`/`-` 当成指数符号 `1e+5` 路径的一部分吞掉，产出非法 token」，**已被上述更正取代**，保留以备查证。

</details>

<details>
<summary><b>💥 影响面（含 klex / kparse）</b></summary>

`libs/koala/klex` **忠实镜像了 `koala.l`**，所以 `klex.tokenize("a-1")` 也产出 `ID / INT_LITERAL(-1)`（已实测）。

基于 klex 的 kparse 若不做处理，会在同样的输入上失败；而写 koala 代码时也必须给 `+`/`-` 两侧加空格。

</details>

<details>
<summary><b>🩹 绕过办法</b></summary>

运算符两侧加空格。

</details>

---

### 修复方案

<details>
<summary><b>🛠 候选修法（倾向 B）</b></summary>

- **A**：词法层去掉数字规则的 `[+-]?`，符号一律作为独立运算符 token，负数字面量由 parser 的一元 `-` 折叠（需要同步 `ps->sign` / `YY_INT_VALUE` 的符号处理）。
- **B**：保留符号规则，但**仅当前一个 token 不是「操作数结束符」**时才吞符号（klex 的 `_is_ender` 集合：ID / 字面量 / `)` / `]` / `}` / `?` / `!` / `_` 等）。

**B 改动小且与 ASI 复用同一判定，倾向 B。**

> 原建议修法（2026-11-21，基于已被更正的根因）：仅在刚吃掉 `e`/`E`（十六进制的 `p`/`P`）之后才接受 `+`/`-` 作为符号，其余位置立即回退，交回主扫描循环作为独立运算符 token。

</details>

<details>
<summary><b>🧪 建议测试</b></summary>

`test/test-run/test_lex_num_sign.kl`，覆盖 `2+3`、`2-3`、`[i+1, i-1]`、`(2+3)`、`2*3`，并保证 `1e+5` / `1e-5` / `0x1F` 仍正确。

追加（2026-11-22）：`a-1`、`a+1`、`arr[i-1]`、`f(n-1)`、`-1`、`a - -1`、`1e-5`、`0x1F`。

</details>

<details>
<summary><b>📌 kparse 侧暂定策略</b></summary>

**不改 klex**，parser 在「操作数 + 带符号 INT/FLOAT 字面量」相邻时把字面量拆成二元 `+`/`-` 与无符号字面量，保证能解析编译器目前拒绝的写法（**超集，不产生误拒**）。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 发现于 Phase 2 基准准备阶段，用户确认先记录、待后续处理。2026-11-22 klex/kparse 评审期间补充标识符形态并更正根因。

</details>