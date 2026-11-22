# 2026-10-28 发现的 bug（to fix）

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— 第 6/7/8/10 项已解决；**第 2/3（内存泄漏）、4（泛型 tuple 不变）、5（错误恢复噪音）、9（`?.` 被禁用）仍开**

> 来源：`docs/Koala_TODO.md` §33（to fix）
>
> 在 `koala.y` 恢复 / 内存整治与 tuple 工作中发现。与 tuple 相关的 bug 在 [#119](issue-119.md)（§32.4 第 9–11 项：tuple 上的 `==`、`any` 局部做 `is` 崩 VM、`if v != nil` 下 `v = nil` 的 isel 失败）。

---

### 详细描述

<details>
<summary><b>📌 第 1 项：`a ?: b` 已移除（2026-10-29）</b></summary>

`?:` token（`OPT_DEF`）与 `or_expr OPT_DEF or_expr` 规则已从 `koala.l` / `koala.y` 删除；那条规则以前会**静默地求值为 `a`**。`?:` 与其它语言里的 Elvis 含义冲突。null 合并的 `a ?? b` 以后可能加。

</details>

<details>
<summary><b>💧 第 2 项：每次编译都有内存泄漏（parser 之外）· 未修</b></summary>

对合法程序跑 `valgrind --leak-check=full koalac` 报约 22–29 KB "definitely lost"。在 `%destructor` 工作与 tuple / map 的 free 修复之后，parser 与 AST 已经不漏。其余：

- **`.klc` 加载**：`read_intf_table()`（`klc.c:1460`，3 个块约 36 KB）与 `klc_const_to_literal()`（`read_klc.c:310`，约 5 KB）；
- **语义 pass**：`handle_valist_and_dfl_args()`（`parser_expr.c`，旧的 `KeyWordExpr` 节点被丢弃，见它的 `// TODO: free old args`）、`parse_tuple()`（`tp_args` 向量）、`find_or_add_instance()`（`parser_generic.c:511`）、`stbl_new()`、`str_ndup()`。

</details>

<details>
<summary><b>🏷 第 3 项：语法错误后注解值会泄漏 · 未修</b></summary>

`koala.y` 对 `<prefix_flags>` / `<annot_param>`（注解与 prefix 标志）以及嵌套类型列表向量 `specialized_types_list` / `atom_primitive_type_lists` **没有 `%destructor`**，所以 bison 在错误恢复中丢弃它们时值会泄漏。

当初是**有意**留空的：其中某些规则可能没有完整初始化结构体，对垃圾值调 destructor 会崩。**修法**：让每条规则先完整赋值，再加 destructor。

</details>

<details>
<summary><b>🧬 第 4 项：带泛型实参的 tuple 仍按不变比较 · 未修</b></summary>

逐元素检查（§32 第 1 项的 `compatible_tuple()`）只覆盖具体 tuple 实例（`TYPE_KLASS`）。元素里含类型参数的 tuple 类型是 `TYPE_GENERIC_REF`，走 `compatible_generic_ref()`，那里要求每个实参**严格相等**。所以在 `func f[T](x T) (T?, int)` 里，`return (nil, 1)` 可能仍然失败。

</details>

<details>
<summary><b>🔊 第 5 项：`tuple = x` 之后的错误恢复噪音 · 未修</b></summary>

由于 tuple 赋值规则把 `'(' expr_list ',' expr ')'` 写开了，`tuple = x`（左边是 `tuple` 关键字）是语法错误，但恢复过程会追加连带错误：`"expected '}'."` 与 `"'x' is not found"`。

---

### 修复方案

<details>
<summary><b>✅ 第 6 项：trait 类型的全局变量未 `make_intf` 就存储（2026-10-30 发现 · 已解决 2026-10-31）</b></summary>

```koala
let l = [1, 2, 3]
let s Sequence[int] = l
print(s.len())
// koala: include/runtime/vm_ops.h:430: _eval_frame: Assertion `((callable)->tag > (64 + 1))' failed.
```

模块 `__init__` 发射 `global_set @s std/builtin.Sequence<j>, %0 std/builtin.list<j>`：存的是**裸对象**而不是接口值，于是每次经该全局的方法调用都在非接口对象上派发。局部 trait 同样失败（顶层的 `let n Named = W()`）。同一个代码在函数里是好的（`make_intf ... to std/builtin.Sequence`）。

**修法**：在 `global_set` 之前（以及后续对该全局的赋值）用 `_build_obj_intf_upcast()` 转换初值，与局部路径一致。在此之前 `test/test-run/test_intf_ext_generic_inherit.kl` 一直把值放在局部里。

**Resolved (2026-10-31)**：`emit_ir_var_decl()` 的全局分支在 `global_set` 之前用 `_build_obj_intf_upcast()` 转换初值；后续赋值由第 7 项覆盖。测试：`test/test-run/test_intf_assign.kl`。

</details>

<details>
<summary><b>✅ 第 7 项：给已存在的 trait 类型变量赋类值会跳过 `make_intf`（2026-10-31 发现 · 已解决同日）</b></summary>

```koala
var x Fly = Bird()   // fine: make_intf ... to Fly
x = Bird()           // move %x Fly, %2 Bird -- the bare object is stored
print(x.fly())       // koala: vm_ops.h:430: Assertion `((callable)->tag > (64 + 1))' failed.
```

声明会转换值，而**后来的普通赋值不会**：`emit_ir_simple_assignment()` 原样 move `rhs->ir_val`。被 `var (f Fly, n) = ...` 绑定的名字之后再赋值也一样。**修法**：在赋值路径存之前用 `_build_obj_intf_upcast()` 转换（局部、字段、全局），与声明一致。很可能与第 6 项同根。

**Resolved (2026-10-31)**：`emit_ir_simple_assignment()` 先把值转换成目标类型，覆盖局部、全局、字段与 list 元素。索引存储的目标类型是 `__setitem__` 的 value 形参，由 `parse_index_store()` 记在新的 `IndexExpr.value_ts` 里（索引表达式自身的类型是下标类型）。同 trait 的 trait 值原样存；派生 trait 上转成基 trait。测试：`test/test-run/test_intf_assign.kl`。

</details>

<details>
<summary><b>✅ 第 8 项：optional trait 之间 `A?` → `C?` 不上转，调错方法（2026-10-31 发现 · 已解决同日）</b></summary>

```koala
trait B { func b1() str }
trait C { func c1() str }
trait A : B & C { func a1() str }
// Foo : A implements all three
let a A? = Foo()
let c C? = a
print(c!.c1())    // prints "b1": B's method
```

IR 是 `move %c C?, %a A?`：没有 `upcast_intf`。`_build_obj_intf_upcast()` 对 optional 源类型 `A?` 找不到符号，直接返回不做转换（它的第 5 步）。值保留 A 的 itable，而调用取的是其中 C 的 slot 0 —— 也就是 A 的 `b1`。

**只有一个基 trait 时这个 bug 被掩盖**，因为基 trait 的方法总是派生 trait 表的前缀。非 optional 的 `let c C = a` 是正确的（`upcast_intf ... to C`）。

**修法**：optional 源也要解包，在 nil 检查下发射上转（nil 保持 nil），覆盖声明、赋值、实参、返回值。

**Resolved (2026-10-31)**：`_build_obj_intf_upcast()` 把 optional 源交给新的 `_build_opt_obj_intf_upcast()`：当 `_intf_upcast_needed()` 说内层类型需要转换时，它发射一个结果局部（`nil`）、一次 nil 检查，并在非 nil 路径上 `cast A? to A` + `upcast_intf`（类则是 `make_intf`）写入该局部。所有调用方都走它（声明、全局、赋值、实参、返回值、tuple 元素、字段）；同 trait（`A?` → `A?`）仍是普通 move。测试：`test/test-run/test_intf_optional_upcast.kl`。

</details>

<details>
<summary><b>⏸ 第 9 项：optional 链 `?.` 配方法调用在 irgen abort（2026-10-31 发现 · 已改为明确报错，待实现）</b></summary>

```koala
let c C? = C()
print(c?.m())     // koalac: irgen.c:668: update_call_args:
                  //         Assertion `proto_ts->kind == TYPE_PROTO' failed.
```

trait（`a?.hello()`）与 `str?`（`s?.len()`）同样；字段上的 `c?.v` 也失败（`koala` 报 "compilation failed" 而 `koalac` 退出 0，未深查）。手册在 Optional chaining 下写着 `b?.get()` 是支持的。**目前还没有测试用 `?.` 配调用。**

**Disabled (2026-10-31)**：`parse_dot()` 现在报 `'?.' is not supported yet, use 'if let' or '!.'`，而不是让 irgen abort；文法规则、`DOT_OPTIONAL` 及其类型推断都保留，手册也注明尚不支持。

**以后实现**：方法调用与字段访问，以及整条链的 nil 短路；然后删掉这条错误。当前由 `test/test-kl/test_opt_dot_disabled.kl` 测试。

</details>

<details>
<summary><b>✅ 第 10 项：调用给的实参过多不报错（2026-10-31 发现 · 已解决同日）</b></summary>

```koala
func one(x int) int = x
one(1, 2)          // compiles without an error
typeof(1, 2)       // the same for builtins
hash(1, 2)
```

实参过少会报（"too few arguments in function call."），多余的被静默接受，用户函数与 builtin 都一样。

**Resolved (2026-10-31)**：`check_call_args()` 在第一个多余实参处报 "too many arguments in function call."。打开它之后暴露出：类字段初始化式在它们调用的签名被解析**之前**就被访问（在后续类的方法之前、在所有顶层函数之前），所以它们的实参从未被检查。现在字段有了自己的 pass：`kl_parse_ast_klass_field_meta()`（`koalac.c`），排在方法与顶层函数签名之后、函数体之前。

测试：`test/test-kl/test_call_too_many_args.kl`、`test/test-kl/test_field_init_call_errors.kl`、`test/test-run/test_field_init_call.kl`；`test/test-kl/test_call.kl` 现在也期望它那三条真实的「多余实参」错误。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#119](issue-119.md)（§32）：tuple 字面量与期望类型，第 9–11 项即本节的 tuple 侧 bug；第 4 项（泛型 tuple 不变）与 §32 第 2/3 项同源。
- [#113](issue-113.md)（§49）：`__add__` 等运算符的副作用语义。
- §34（泛型 bound 实参未转接口，fixed 2026-11-07）与本节第 6/7/8 项同属「trait 值未 `make_intf`」这条链路。

</details>
</details>