# 27. 参数/实参列表右括号前换行会被 ASI 插入分号而报错

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 形参列表：ASI 既有规则，已按写法规避，报错文案/文法容忍未改
- [ ] 未修复 —— 调用实参列表同样触发（`print(1` 换行 `)`），且与 `[]`/`{}` 字面量容忍度不一致
- [ ] 待补测试 —— 实参列表右括号前换行，及与列表/字典字面量的对照

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

多行写函数签名时，若最后一个形参单独占一行、`)` 另起一行，报 `expected a valid function declaration.`：

```
func f(
    a int, b bool
) {}
```

把 `)` 提到最后一个形参同行（`a int, b bool) {}`）即通过；**`(` 之后换行是允许的**。

</details>

<details>
<summary><b>🔍 现象（调用实参列表）</b></summary>

同一规则在**调用实参列表**上同样触发，且不限于类型关键字——末实参是普通字面量即可：

```
func main() {
    print(1
    )
}
```

报 `expected an expr list or ')'`，随后连带 `expected '}'`。把 `)` 提到 `1` 同行即通过。

**关键不一致**：列表/字典字面量的闭合括号独占行是**允许**的，唯独调用的 `)` 不行：

| 写法 | 结果 |
| --- | --- |
| `print(1` 换行 `)` | ❌ |
| `print(1,` 换行 `)` | ❌（尾逗号也救不了） |
| `let x = [1,` 换行 `2` 换行 `]` | ✅ |
| `let d = {"a": 1,` 换行 `"b": 2` 换行 `}` | ✅ |
| `f("x",` 换行 `[1, 2])` | ✅（`)` 紧跟末实参） |

`(` 之后换行、实参之间换行都允许，只有**末实参与 `)` 之间的换行**致命。

</details>

<details>
<summary><b>🧠 根因</b></summary>

`bool`/`int`/`str` 等**类型关键字都在 `koala.l` 的 `semicolon()` 结束符集合里**，形参后的换行触发 ASI，插入的 `;` 落在 `param_list` 与 `)` 之间，文法不接受。

形参类型是**自定义类名（ID）**时同样触发。

调用实参侧同理：`src/parser/koala.l:22-28` 的集合还含 `INT_LITERAL`、`FLOAT_LITERAL`、`STRING_LITERAL`、`CHAR_LITERAL`、`BYTES_LITERAL`、`ID`、`SELF`、`TRUE`、`FALSE`、`NONE` 等，所以**任何以字面量或标识符结尾的末实参**后换行都会插 `;`。

差异只在文法容忍度：列表/字典字面量的元素表容忍尾随 `;`，调用实参的 `expr_list` 不容忍，于是同一个 ASI 在 `[]`/`{}` 里无害、在 `()` 里报错。

</details>

<details>
<summary><b>💥 影响</b></summary>

**不是缺陷而是 ASI 的既有规则**，但写多行签名时很容易踩到，且报错位置指向 `(` 而不是换行处，**不易定位**。

**实际阻塞**：`koala-libs/examples/rest/openai_demo.kl` 因此无法编译——`client.chat.completions.create(...)` 用了「每个实参独占一行、`)` 也独占一行」的主流排版。报错落在列表字面量所在行，极易误判为列表/字典语法问题：2026-11-24 排查时先怀疑字典字面量与尾逗号，逐步收窄到 `print(1` 换行 `)` 才定位。该示例的编译失败是**既有问题**，与当日的注释修改无关（用原注释编译报错行号仅差 1）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

报错文案提示「参数列表不能以换行结束」，或让 `param_list` **容忍一个尾随 `;`**。

调用实参侧建议**对齐字面量的容忍度**：让 `expr_list` 也接受尾随 `;`（与 `[]`/`{}` 一致），`)` 独占行的常见排版即可用。若判定必须保持 ASI 语义，则至少把报错位置指到换行处并说明原因，而不是指向 `(` 或列表字面量。

</details>

<details>
<summary><b>🧪 建议新增测试</b></summary>

- 实参侧现状固化：`not %koala -c` 跑 `print(1` 换行 `)`，断言 `expected an expr list or ')'`；修复后转正向用例。
- 对照组（防改坏字面量侧）：`let x = [1,` 换行 `2` 换行 `]` 与 `let d = {"a": 1,` 换行 `"b": 2` 换行 `}` 必须通过。
- 形参侧回归：`func f(` 换行 `a int, b bool` 换行 `) {}`。
- 端到端：`koala -c koala-libs/examples/rest/openai_demo.kl` 应通过（当前失败）。
- 注意 `test/parse_args.test` Case 21 覆盖的是 `print(1` **未闭合到 EOF**，与本条「换行后闭合」不同，不能替代。

</details>

<details>
<summary><b>🩹 绕过办法</b></summary>

`)` 与最后一个形参/实参同行（调用侧同理：`print(1)` 而非 `print(1` 换行 `)`）。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-22 记录于 kparse 编写期间，已按「`)` 与最后一个形参同行」的写法规避。

2026-11-24 补充调用实参列表场景：改 `koala-libs/examples/rest/openai_demo.kl` 头部注释时做 `koala -c` 校验，发现该示例本就编译失败，收窄后确认与本条同源，故并入本条而非新开编号。

</details>