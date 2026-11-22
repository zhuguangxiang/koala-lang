# 20. `list[uint8]` 的整数字面量收窄规则前后不一致

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 三条路径两种结果，已用辅助函数绕过，待讨论

---

### 详细描述

<details>
<summary><b>🔍 现象：三条路径两种结果</b></summary>

同样把 `int64` 字面量放进 `uint8` 元素位置：

| 写法 | 结果 |
|---|---|
| `var a = list[uint8](65, 66)` | ❌ `error: Incompatible type: expected 'uint8', but got 'int64'.`（每个实参各报一次） |
| `var b list[uint8] = [65, 66]` | ❌ `error: Types of two sides are not matched. lhs: list[uint8] =/= rhs: list[int64]`（列表字面量先自行推导成 `list[int64]`，再整体比对，不做逐元素收窄） |
| `var c list[uint8] = []; c.append(65)` | ✅ **编译通过且运行正确**（`T` 已绑定为 `uint8` 的方法调用反而接受了 `int64` 字面量） |

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

```
func main() { var a = list[uint8](65, 66); print(a.len()) }
```

报错；把同一行换成下面则输出 `1`：

```
func main() { var c list[uint8] = []; c.append(65); print(c.len()) }
```

</details>

<details>
<summary><b>💥 影响</b></summary>

构造 `uint8` 序列（`frozenbytes(data Iterable[uint8])`、`Buffer`、`bytes.fill` 等入口）时，**最自然的两种写法都不可用**，只能写 `list[uint8](uint8(65), uint8(66))` 或先建空表再 `append`。

本轮 `test_frozenbytes.kl` 因此用 `byte_list(s str)` 辅助函数绕开。

</details>

<details>
<summary><b>🧠 初步分析</b></summary>

疑为三处的**类型检查入口不统一**：

- 泛型构造的变长实参走严格 `compatible` 检查；
- 列表字面量走「先推导整体类型再赋值兼容」；
- 已实例化泛型的方法实参走了**允许字面量收窄**的另一条路径。

需要定位后统一，而不是单独放宽某一处。

</details>

<details>
<summary><b>❓ 未定性</b></summary>

尚未判定 `append` 接受 `int64` 是「字面量收窄」的**有意设计**还是**漏检**（若存入的值超出 `uint8` 范围会怎样，**未测**）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

先确定**语言规则**（整数字面量能否隐式收窄到定长整型、在哪些位置），再让三处**共用同一判定**。

若允许收窄，则列表字面量应**按声明的元素类型推导**而非先推 `int64`。

</details>

<details>
<summary><b>🧪 建议测试</b></summary>

`test/test-ut/test_uint8_literal_narrowing.kl`，覆盖上述三条路径 + 越界字面量（`append(256)` / `append(-1)` / `list[uint8](256)`），锁定「哪些位置允许收窄、越界如何报错」。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 发现于 frozenbytes 独立类型实现的测试编写期间，已用辅助函数绕过，待讨论。

</details>