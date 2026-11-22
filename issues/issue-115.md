# 变长参数的打包 / 解包：实现现状与缺陷（待修 / 待设计）

← 返回 [问题列表](issues.summary.md)

- [ ] 待修 —— 缺陷 A / B / C 待修，缺陷 D 作者决定暂不解决；**设计未定案，未施工**

> 来源：`docs/Koala_TODO.md` §91（2026-11-12，待修 / 待设计）
>
> 起因是修「变长参数静态下标 ≥ 1 报越界」时顺带挖出的转发与 `!` 解包问题。先把当前实现的模型写清楚，再列缺陷。

---

### 详细描述

<details>
<summary><b>🧩 当前模型：变长槽 = 一个位置实参，持有一个 tuple 对象（91.1）</b></summary>

- **打包**在 `handle_valist_and_dfl_args()`（`src/parser/parser_expr.c:1967`）：把第 `valist_index` 个及之后的位置实参收进 `vec`，再 `expr_from_tuple(vec)` 造成**一个** tuple 表达式塞进 `real_args`（`:2035`）。零变长实参时也补一个空 tuple（`:2047`），注释（`:2042-2045`）说明理由：变长形参恒占一个位置槽，实参个数必须与 prototype 对齐。
- 因此被调方看到的 `xs` 就是一个普通 tuple 值，`xs.len()` / `for x in xs` / `xs[i]` 全部走 tuple 的方法，**运行期没有独立的 va_list 概念**。
- 实测（根目录 `test22.kl`）：

  ```kl
  func pass(xs ...int) int {
      print(xs)                  // → 1 2 3
      print(xs, typeof(xs))      // → (1, 2, 3) <class 'tuple'>
      return 0
  }
  ```

  同一个 `xs`，独占变长槽时被摊开，和别人并列时是一个 tuple 元素。差异来自下面的直通路径。

</details>

<details>
<summary><b>🔀 已存在的「直通」路径 `skip_tuple`（91.2）</b></summary>

`parser_expr.c:2011-2025`：当变长槽**只收到 1 个**实参、且该实参的 `ts` 本身是 `TYPE_VA_LIST` 时，不再包 tuple，直接透传（`skip_tuple = 1`）。这就是 `print(xs)` 输出 `1 2 3` 的原因。两个限制：

- 只在独占整个变长槽时生效；`print(xs, typeof(xs))` 有 2 个实参 → 走打包 → `xs` 成为新 tuple 的一个元素。
- **完全不检查元素类型**，只看「是不是 va_list」。

**关键结论：打包层已经支持「把一个变长值原样交给另一个变长形参」，类型检查层不认这件事。** 下面 A / B 两个缺陷都是这两层不一致的表现。

---

### 修复方案

<details>
<summary><b>❌ 缺陷 A：转发给带类型的变长形参被类型检查拒绝（待修）</b></summary>

- [ ] ```kl
  func second(xs ...int) int { return xs[1] }
  func pass(xs ...int) int { return second(xs) }   // 报错
  ```
  `Incompatible type: expected 'int64', but got '...int64'.`

位置：`check_valist_arg()`（`parser_expr.c:1073`）在 `:1098` 直接拿**元素类型**与实参类型比兼容性；它只有 `!` 解包分支（`:1093`），没有「实参本身是一个 va_list 值」的分支。

实测矩阵（失败只取决于**被调方**的变长形参有没有类型）：

| 调用方 | 被调方 | 结果 |
|---|---|---|
| `...int` | `...int` | 报错 `expected 'int64', but got '...int64'` |
| `...`（无类型） | `...int` | 报错 `expected 'int64', but got '...any'` |
| `...int` | `...`（无类型） | 通过 |
| `...` | `...` | 通过（`test-run/test_valist_zero_args.kl` 即此形态） |

后两行通过**不是**因为有特判，而是被调方元素类型为 `any`，`type_spec_compatible(any, ...int64)` 恒真，把整个 tuple 吞掉了。

**方向**：让 `check_valist_arg` 承认「实参是 va_list / tuple 值且独占变长槽」，与 `skip_tuple` 对齐，同时校验元素类型兼容（`...int` → `...str` 应报错）。

</details>

<details>
<summary><b>❌ 缺陷 B：`!` 对 Sequence 只做了类型层，codegen 是空操作（待修 / 待设计）</b></summary>

- [ ] **类型层**：`parse_bang()`（`parser_expr.c:4068`）在 `:4098-4103` 用 `match_sequence(e->ts, &it_ts, NULL)` 把静态类型改成 Sequence 实例类型（`it_ts` 出参语义见 `typespec.c:845`）；`check_valist_arg:1093` 再用 `arg_ts` 出参取**元素类型**，于是检查通过。
- [ ] **代码生成层**：`emit_ir_bang()`（`src/parser/irgen.c:2278-2300`）在非 optional 时直接 `exp->ir_val = e->ir_val` 返回（`:2290`，注释写的是 shadow variable 的场景），**没有任何摊开动作**。KLR 指令集、`isel.c`、VM 里也都没有用于**调用实参摊开**的指令（`irgen.c:3369` 的 `build_unpack_vars` 是 `let (x, y) = ...` 的解构，与此无关）。即 `!` 的 Sequence 语义目前只是一个类型标注。
- [ ] **实测后果（静默算错，比崩溃更危险）**：

  ```kl
  func n(xs ...int) int { return xs.len() }
  func count_bang(xs ...int) int { return n(xs!) }
  func second(xs ...int) int { return xs[1] }
  func pass_bang(xs ...int) int { return second(xs!) }
  ```
  `count_bang(10, 20, 30)` 得 **1**（应为 3）；`pass_bang(10, 20, 30)` 运行时抛 `Error: tuple index out of range`。`typeof(xs!)` 仍是 `<class 'tuple'>`，值就是原 tuple。

链路：`xs!` 的 `ts` 已被 `parse_bang` 改成 Sequence 而非 VA_LIST → `skip_tuple` 不触发 → 整个 tuple 被当成**一个元素**再包一层 → 被调方 `len()` 为 1。

**方向**：`!` 需要真正的下降。打包时把 sequence 的元素展开进 tuple（运行期长度未知，需要 runtime 侧的 tuple 拼接 / 扩展支持），或引入显式 spread 指令。**属设计题，不是补一个分支。**

</details>

<details>
<summary><b>❌ 缺陷 C：`xs!` 能否编译取决于编译顺序（偶然而非设计，待修）</b></summary>

- [ ] `match_type_spec()`（`typespec.c:829-832`）靠 `get_symbol_by_id(ts->sym_id)` 找符号；va_list 的 `sym_id` 初值是 -1（`va_list_type_spec()`，`typespec.c:732`），由 `get_type_symbol()`（`parser.c:437`）**惰性**填写。于是 `xs!` 是否被认成 Sequence，取决于之前有没有别的代码碰过同一个 va_list TypeSpec。

实测三个最小形态（同一函数体，只差前置语句）：

```kl
// w1：xs! 是第一次使用 → 编译报错
func pass(xs ...int) int { print(xs!, typeof(xs!))  return 0 }
// error: only optional type/Sequence's subtype can use bang operator.

// w2：先 xs.len() → 通过，且 xs! 的值仍是整个 tuple
func pass(xs ...int) int { print(xs.len())  print(xs!, typeof(xs!))  return 0 }
// → 3 / (1, 2, 3) <class 'tuple'>

// w3：先 xs[0] → 同 w2
func pass(xs ...int) int { print(xs[0])  print(xs!, typeof(xs!))  return 0 }
// → 1 / (1, 2, 3) <class 'tuple'>
```

**方向**：va_list 的类型符号应在 `resolve_type` 阶段就确定，不要依赖惰性填 `sym_id` 的副作用。

</details>

<details>
<summary><b>⏸ 缺陷 D：`tuple.__getslice__` 未实现（2026-11-12 作者决定：暂不解决）</b></summary>

- [ ] 任何 tuple 切片都在运行时抛 `func 'tuple::__getslice__' is not implemented!`，**不限于变长参数**：

  ```kl
  let t = (1, 2, 3)
  print(t[1:])        // Error: func 'tuple::__getslice__' is not implemented!

  func tail(xs ...int) int {
      var s = 0
      for x in xs[1:] { s = s + x }   // 同样抛错
      return s
  }
  ```

`tuple_methods[]`（`src/objects/tupleobj.c:60-66`）只注册了 `to_list` / `fmt` / `len` / `__getitem__`，没有 `__getslice__`。前端能过（`parse_slice_load` 认 tuple 是 Sequence），所以是**纯运行期缺口**，编译期无诊断。

注：`demo/func_type.kl` 的 `reduce` 用 `xs[1:]` 能跑，是因为那里的 `xs` 是 `list[int]`；变长槽是 tuple，同样的写法就炸。`for x in xs` 直接迭代 tuple 是正常的，只有切片不行。

**方向（未施工）**：给 tuple 补 `__getslice__`，返回新 tuple；需先定下越界 / 负下标 / 空切片的语义，与 [#54](issue-54.md)（§11，`OP_SEQ_GET_IMM` / `OP_SEQ_SET_IMM` 负下标语义未定义）一并考虑。

</details>

<details>
<summary><b>⚠️ 设计前必须先看：既有负向用例把「`!` 不摊开」钉住了（91.7）</b></summary>

- [ ] `test/test-kl/test_list.kl:54,57-58`：`let l6_1 = list(100, l2!, 200)` 之后 `l6_1[1][0] = 100` 期望报 `type 'int64' is not subscriptable (missing __setitem__)`。
- [ ] `test/test-kl/test_infer_new.kl:66-68`：`A(100, 200, "hello", l!, v5 = 1, v6 = false)` 之后 `a6.f2[0] = 100` 期望报同样的错。

这两条都建立在「`l!` 只贡献**一个**元素」的现状上。真要实现 spread，**必须先决定这两个用例怎么改**，不能直接改实现。

</details>

<details>
<summary><b>✅ 已修 / 已固化的部分（91.8）</b></summary>

- 变长参数**静态下标 ≥ 1** 已修（原 C1）。根因：`get_valist_symbol()`（`parser.c:421`）把变长参数映射成 `tuple[元素类型]`，只有 1 个类型实参，而 `infer_tuple___getitem__` 拿字面量下标去和 `vector_size(inst_sym->tp_args)`（恒为 1）比。修法：`infer_func_tp` 增加 `recv_ts` 参数（`include/parser/parser.h:230`），`parser.c:53` 对 `TYPE_VA_LIST` 直接返回 `va_list.src`，不做 arity 检查（长度只有调用点才知道）。
  不能给 instance 打标记：`tuple[int64]` 与真实一元组 `(1,)` 是**同一个缓存符号**（`find_or_add_instance` 按 mangled name 缓存），打标记会破坏真实元组的越界检查。
- 用例：`test/test-run/test_vararg_index.kl`（纯类型 `...int` / `...str` / `...float` / 无类型 `...` / 泛型 `...T`，覆盖下标 0/1/2、方法、泛型函数、泛型类方法，并保留动态下标、常量折叠下标、for-each、`len()` 作回归护栏；两条 RUN：默认 + `--ssa`）。
- **该用例目前故意留红**：按作者要求保留了 `pass`（缺陷 A）与 `pass_bang`（缺陷 B）两条待修用例，等设计定案后再动。全量 491/492，唯一红点就是它。注意文件里缺陷 B 被缺陷 A 遮住（`pass` 编译不过，lit 跑不到运行时），**两个缺陷要分开修、分开验**。

</details>

<details>
<summary><b>🔗 关联条目（91.9）</b></summary>

- [#88](issue-88.md)（§90）已记的「可变参数 `...Trait` 接收类实例时 VM 崩溃」（打包 tuple 时元素未 `make_intf`）与本节**同源**，都在 `handle_valist_and_dfl_args()` 的打包处，宜一并设计。
- [#87](issue-87.md)（§89）与 §90 已记的「泛型可变参数 `...T` 推断 `NYI()`」（`parser_generic.c:126` 的 `TYPE_VA_LIST` 分支）也在同一条链路上。
- 切片负下标语义：[#54](issue-54.md)（§11）。

</details>
</details>