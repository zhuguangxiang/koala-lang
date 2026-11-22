# `--inline` 支持闭包 call 与函数类型变量 call（部分完成）

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— immutable local + 单次 `move @func` 已内联；`var` / 多次赋值 / 形参 / 全局仍为 call

> 来源：`docs/Koala_TODO.md` §99（2026-11-13，部分完成）

---

### 详细描述

<details>
<summary><b>🔍 现状（实测，`--dump=inline-ir`）</b></summary>

`src/parser/opt/inline.c` 的 `direct_callee` 只认 `call @func`（操作数 0 直接是本模块的 `KlrFunc`），经过函数类型变量的间接调用不内联：

```kl
func add(a int, b int) int { return a + b }
func run() {
    let f = add
    print(f(1, 2))                  // call %f ...：不内联
    let h = { |x int| x * 2 }
    print(h(3))                     // call %h ...：不内联
    print(({ |x int| x - 1 })(8))   // 直接调用的 lambda：已内联为 sub 8, 1
}
```

IR 里 `%f` / `%h` 是 `let`（immutable）local，唯一一次定义就是 `move %f, @add` / `move %h, @__lambda$0`，**callee 在编译期其实已确定**。

</details>

---

### 修复方案

<details>
<summary><b>✅ 已完成</b></summary>

- [x] **函数类型变量**：`call %v` 中 `%v` 是 immutable local、只有一次定义且为 `move %v, @func`（本模块函数）时，`resolve_func_value` 把 callee 解析为 `@func` 后按直接调用内联。支持 `let f = add`、嵌套 `n()(9)`（内联后的函数体会被重新扫描）、lambda 作为参数传给被内联函数。
- [x] **闭包**：无捕获 lambda 赋给 `let` 后调用，同上（目前闭包本身不允许捕获）。
- [x] 内联后 `%v` 不再使用时，`local` + `move @func` 由后续 DCE 删除。
- [x] 测试：`test/test-run/test_inline_closure.kl`（`var` 函数变量保持 call）。

</details>

<details>
<summary><b>🛠 未做</b></summary>

- [ ] `var` 变量、多次赋值、函数参数、全局 `let callback = {...}` 的函数值：callee 不确定或未处理，仍为 call。
- [ ] 将来闭包允许捕获后，内联需处理捕获环境。

</details>

<details>
<summary><b>🔗 关联</b></summary>

§96（[#90](issue-90.md) 多 bb）、§97（[#91](issue-91.md) 泛型）：解析出 callee 后仍受这两条限制。

</details>