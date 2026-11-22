# 25. 全局 trait 类型变量经 `is` 收窄后使用，ir_cast panic

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— [#23](issue-23.md) 的同族形态，建议合并修复；kparse 已按绕法设计

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

全局变量声明为 trait 类型，在函数里用 `is` 收窄后调用其方法，编译崩溃：`panic: 'ir_cast' op requires a reg value or const`。

- 同一代码把变量改成**局部变量**则正常。
- **不做 `is` 收窄**（直接调用 trait 方法）也正常。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

```
trait S { func t() str }
class A : S {
    pub func t() str = "a"
}
let g S = A()
func main() { if g is A { print(g.t()) } }
main()
```

→ `panic: 'ir_cast' op requires a reg value or const`；把 `let g S = A()` **移进 `main()`** 即输出 `a`。

</details>

<details>
<summary><b>🧠 初步分析</b></summary>

与 [#23](issue-23.md) **同一根因位置** —— `emit_ir_ident` 的 `SYM_SHADOW_VAR` LOAD 路径对全局变量直接取 `origin->ir_val`（`KLR_VALUE_GLOBAL`）送 `klr_build_cast`，未先 `klr_build_get_global` 加载。

#23 是**可选类型判空收窄**，本条是 **trait 类型 `is` 收窄**，触发的是同一条 cast 路径。

</details>

<details>
<summary><b>💥 影响与绕法</b></summary>

所有「全局 trait 类型值 + `is` 收窄」的写法不可用。

写库时的绕法：把 trait 类型的值放在**局部变量或对象字段**里（字段访问后收窄已验证正常），不要放在模块级全局。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

**与 [#23](issue-23.md) 合并修复** —— shadow LOAD 对 `VAR_SCOPE_GLOBAL` 先 `klr_build_get_global` 再按需 cast。

</details>

<details>
<summary><b>🧪 建议测试</b></summary>

`test/test-run/test_global_trait_narrow.kl`，覆盖全局 trait 变量 `is` 收窄后调方法/取字段、`else` 分支、循环内收窄，以及对照组（局部变量、对象字段）。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-22 发现于 kparse 的 CST 节点模型选型探针（trait 类型 + `is` 分派），**未修**，待讨论。kparse 已按绕法设计（节点值只存在于局部与字段中）。

</details>