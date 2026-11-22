# 整数扩宽后 `typeof` 不一致：编译期折叠 vs 运行期

← 返回 [问题列表](issues.summary.md)

- [ ] 待讨论 —— 三个方案未定；测试暂用宽松 CHECK

> 来源：`docs/Koala_TODO.md` §101（2026-11-14，待讨论）

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

```kl
func return_int64(v int8) int64 { return v }
let r = return_int64(int8(-5))
print(typeof(r))        // 不内联：int8；--inline：int64
```

- irgen 对 `return v` 的隐式扩宽生成 `cast %v int8 to int64`。运行期 isel 的 `int_need_cast` 判定扩宽无需转换，值保留 `int8` tag（符合设计文档 §5「A widened value keeps its own type at run time」）。
- 内联后 cast 的输入成了常量 `-5 int8`，const-copy-prop 把它折叠成 `-5 int64`，`typeof` 变成 `int64`。
- **不内联也存在**：`int64(int8(-5))`（常量折叠）是 `int64`，`int64(a)`（运行期）是 `int8`。

</details>

---

### 修复方案

<details>
<summary><b>❓ 可选方案（未定）</b></summary>

- [ ] **A**：只让 `cast` 指令（显式 `int64(x)`、return 的隐式扩宽）在运行期真正改 tag；传参 / let / list 仍保留窄 tag。
- [ ] **B**：所有隐式扩宽都改为目标类型（`typeof` 恒等于声明类型），需**推翻设计文档 §5 的这条规则**，改动大。
- [ ] **C**：维持设计文档 §5：const-copy-prop 不折叠「运行期无效果的整数扩宽 cast」，两条路径都是 `int8`。

</details>

<details>
<summary><b>🧪 测试</b></summary>

`test/test-run/test_int_mix.kl`：`return_int64` / `return_uint64` 两行原 CHECK **已注释**，暂用 `{{int8|int64}}` / `{{uint8|uint64}}` 同时接受两种结果；**定案后收紧**。

</details>