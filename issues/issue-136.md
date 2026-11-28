# 136. 本模块泛型实例上的 `is` / `as` 生成自导入，报 circular dependency

← 返回 [问题列表](issues.summary.md)

- [x] 已定位：`emit_ir_is` / `emit_ir_as` 把 `InstanceSymbol` 的 `ir_val`（NULL）当类对象传给 `klr_build_is` / `klr_build_as`，代码生成把本模块的类当外部类登记成导入
- [ ] 修复：`is` / `as` 改用 `switch_target_klass_ir()` 取类对象，与 `switch ... case is` 一致
- [ ] 测试：新增 `test/test-run/test_is_as_generic_no_self_import.kl`，断言 stderr 不出现 `circular`

> 来源：编写 union 测试（`test/test-run/test_union_supported.kl`）时发现，2026-11-28

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

在模块 `M` 中对**本模块**泛型 class 的实例做 `is` / `as`，运行时 stderr 多打印一行：

```
warning: circular dependency detected: M
```

程序结果正确。会触发的写法：

| 写法 | 是否触发 |
| --- | --- |
| `x is Box[int]` | ✅ 触发 |
| `x as Box[int]` | ✅ 触发 |
| union 参数上的 `if x is Box[int]` | ✅ 触发 |
| `switch x { case is Box[int]: ... }` | ❌ 不触发 |
| `x is Plain`（非泛型 class） | ❌ 不触发 |

</details>

<details>
<summary><b>🔁 最小复现</b></summary>

```kl
class Box[T] {
    var v T
    func __init__(v T) { self.v = v }
}

func main() {
    let a any = Box(1)
    print(a is Box[int])                 // true，同时 stderr 多一行 warning
    print((a as Box[int]) != nil)        // true，同上
}
```

</details>

<details>
<summary><b>🧠 根因</b></summary>

运行时调用链（gdb 确认）：

```
_load_module(M)
  └─ resolve_import(M)              load_module.c:673，此时 M 还没置 MOD_FLAGS_READY
       └─ kl_load_module("M")       导入表项 {IMPORT_KIND_TYPE, path="M", name="Box"}
            └─ _find_module("M")    找到 M 自己，但未 READY，打印 warning
```

导入表里这一项是模块**导入自己的类型**，来源在编译器：

1. `src/parser/irgen.c` 的 `emit_ir_is` / `emit_ir_as` 调 `klr_build_is` / `klr_build_as` 时，类对象参数传的是 `target_sym->ir_val`；随后才把 `target_sym` 换成 `origin`，但只用于 `target_path` / `target_name`。
2. 目标是 `Box[int]` 时 `target_sym` 是 `InstanceSymbol`，它的 `ir_val` 为 NULL，因此 `insn->target_kls == NULL`（gdb：`is Box[int]` / `as Box[int]` 为 `0x0`；`is Plain` 为 `KLR_VALUE_KLASS`）。
3. `src/parser/backend/lower_operands.c` 的 `lower_is_as_opers` 遇到 `target_kls` 为空就当外部类处理，调 `mach_import_add_klass(m, "M", "Box")`，把本模块的类登记成了导入。
4. `switch ... case is Box[int]` 走 `switch_target_klass_ir()`（`irgen.c:3375`），先换成 `origin` 再按需创建 IR 值，所以不触发。

`src/runtime/load_module.c` 的这条 warning 本身是正确的，不需要改。

</details>

<details>
<summary><b>📌 影响</b></summary>

- 所有对本模块泛型实例做 `is` / `as` 的代码都会打印这行 warning；test-run 只检查 stdout，回归测试一直没发现。
- 每个这样的模块多一项导入，多占一个类型槽位。
- 本模块的类型被当成外部类型解析，只是靠 `kl_mo_find` 按名字查找碰巧找对。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

`src/parser/irgen.c` 的 `emit_ir_is` / `emit_ir_as` 改用已有的 `switch_target_klass_ir(ps, target_sym)` 取类对象，替换 `target_sym->ir_val`。三处（`is` / `as` / `switch`）走同一条路径：

- 本模块 class → `KLR_VALUE_KLASS`，不再生成自导入；
- 外部模块 class → `KLR_VALUE_EXT_KLASS`，行为不变。

`switch_target_klass_ir` 需移到 `emit_ir_is` 之前声明。

</details>

<details>
<summary><b>🧪 建议测试</b></summary>

新增 `test/test-run/test_is_as_generic_no_self_import.kl`，`RUN` 行合并 stderr：

```kl
// RUN: koala %s 2>&1 | filecheck %s
// CHECK-NOT: circular
```

覆盖 `is Box[int]`、`as Box[int]`、union 参数上的 `is Box[int]`，每个都 `CHECK` 结果。

</details>
