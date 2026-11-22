# 数值类型字符串解析 API（from_str 家族）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 设计定案，**实现未开始**

> 来源：`docs/Koala_TODO.md` §14（2026-09-12 建立）
>
> 设计定案见 `Koala_Design_Overview.md` §5.2 / §6。

---

### 详细描述

<details>
<summary><b>🔍 现状（源码核实）</b></summary>

- `int64.__init__(x int64 | uint64 | float64 | str, base = 10)`、`uint64` 同形、`float64.__init__(x int64 | uint64 | float64 | str)` —— 三个主类型构造器 union **仍含 `str`**。
- `intobj.c` 中 `int_init` / `str_to_int` 整段被注释（lines 48-119）—— 构造器路径上的 str 解析**未实现**，`int64("100")` 当前命中 not_impl。
- `str.to_int` / `to_int_or` / `to_float` / `to_float_or` 在 `strobj.c` 中是活的（`_str_to_int` 等，调用 `str_to_int` helper）—— `_or` 家族的既有先例。
- 窄类型（int8/16/32、uint8/16/32、float16/32）的 `__init__` 已是纯数值 union（`@intrinsic`），无需改动。
- src 中**不存在任何 `from_str`**。

</details>

<details>
<summary><b>📝 后续变更</b></summary>

**Update (2026-10-31)**：`str` 与 `base` 已从 `int64` / `uint64` 构造器移除（`number.kl`）；`int("12")` / `int64("ff", base = 16)` 现在是**调用点的类型错误**，而不再是 `emit_int_call()` 的 NYI abort，且 `emit_int_call()` 只做数值 cast。

`float64.__init__` 保留了 `str`，因为 `float64("inf")` / `"nan"` / `"3.14"` 字符串字面量在编译期折叠（今天唯一能写 inf / nan 的方式，见 §20 / §22）；运行期的 str 已报 "use 'to_float()'"。

**仍开放**：`from_str_or` / `try_from_str`（当前没有带 base 的解析：`str.to_int()` 只支持 base 10）。

</details>

<details>
<summary><b>🎯 目标形状</b></summary>

每个主数值类（int64 / uint64 / float64）：

```koala
@native
pub func __init__(x int64 | uint64 | float64) {}              // 去掉 str，纯数值 union → cast op

@native
pub static func from_str_or(s str, fallback int64, base = 10) int64 {}

@native
pub static func try_from_str(s str, base = 10) int64? {}
```

- `from_str_or`：**主形态**，裸返回可内联算术；fallback 必传无隐式默认（nothing hidden）。
- `try_from_str`：**检测形态**，`T?` + narrowing。
- 无裸 `from_str`；`base ∉ [2,36]` → panic（bug 类）。
- float64 无 base：`from_str_or(s, fallback) float64` / `try_from_str(s) float64?`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 实现清单</b></summary>

- [ ] `number.kl`：int64/uint64/float64 的 `__init__` 去掉 `str`（和 base）（int64 / uint64 已于 2026-10-31 完成；float64 因字面量折叠保留）；加 `from_str_or` / `try_from_str` static 声明 + doc comment。
- [ ] `intobj.c` / `floatobj.c`（uint64 如有独立文件）：实现 `_int64_from_str_or` / `_int64_try_from_str` 等 native（复用 `strobj.c` 的 `str_to_int` / `str_to_float` helper，或抽到公共位置）。
- [ ] MethodDef 表注册新 static 方法。
- [ ] `intobj.c`：删除注释掉的 `int_init` / `str_to_int` 死代码。
- [ ] static 方法的 klc 持久化 / 调用路径验证。
- [ ] 测试：`test-run/test_from_str.kl`（`from_str_or` 各 base、fallback、`try_from_str` narrowing、base 非法 panic）。

</details>

<details>
<summary><b>🔗 与既有 API 的关系</b></summary>

- `str.to_int` / `to_int_or` / `to_float` / `to_float_or`：**legacy 冻结不动**。将来是否迁移到 `from_str_or` 形态单独立案。
- `int64.to_float()` / `float64.to_int()`（§13.6 NYI 1b/2b）：数值→数值，与 from_str（str→数值）正交，各自独立验证。

</details>

<details>
<summary><b>📚 设计依据</b></summary>

- minimize-panic 原则（`Koala_Design_Overview.md` §6）。
- 显式回退解决安全 vs 易用（§5.2）：fallback 必传 = 失败决策写在调用点 = nothing hidden。
- 模式演化：panic+`_or`（旧）→ `_or`+`try_`（新）。

</details>