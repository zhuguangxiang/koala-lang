# 26. 泛型返回约束 trait 时交出裸对象，调用即断言崩溃

← 返回 [问题列表](issues.summary.md)

- [x] 返回路径（原始最小复现）已修复 —— 新增 `OP_DYNAMIC_CAST`，2026-11-22
- [ ] 泛型体内对容器元素直接调 trait 方法（`return items[0].k()`）—— 仍崩，属 `docs/Koala_TODO.md` §8
- [ ] 泛型体内 `out.append(items[0])` 进 trait 容器后在外部调用 —— 仍崩，同 §8
- [ ] 补回归：常量池 >256 且含泛型 return cast
- [ ] 跨模块同名 trait 的 itab 歧义（方案 A 的已知代价）

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`func f[T : Trait](...) Trait` 用具体类实例化时，返回的 trait 类型值是**裸对象（没有 itab）**，对它调用任何方法立即断言崩溃。解释器与 JIT 都崩：

- 默认 JIT：`src/runtime/eval.c:436: kl_jit_call_dyn: Assertion '((args)->tag > (64 + 1))' failed.`
- `KOALA_JIT=0`：`include/runtime/vm_ops.h:477: _eval_frame:` 同一断言。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

把返回类型 `N` 改成 `T` 就正常。

```
trait N { func k() int }
class P : N {
    pub func k() int = 1
}
func first[T : N](items list[T]) N {
    return items[0]
}
func main() {
    let items = list[P]()
    items.append(P())
    let p = first(items)
    print(p.k())
}
main()
```

</details>

<details>
<summary><b>🧠 根因</b></summary>

容器里的 bounded `T` 与形参 `T` 运行时表示不一致。

- 一个类型参数至多一个 bound（`parser.c:3582`）。`T : B` 的值在擦除版泛型体里就按 `B` 的接口值使用：调用方 `try_to_build_tp_args_cast`（`irgen.c:1025`）把**类型恰为 `T`** 的实参统一转成 `B` 的接口值（class → `make_intf`，trait → `upcast_intf`）。所以形参 `T → B` 不需要转换。
- 但 `list[P]` 传给 `list[T]`（`T : N`）时，**元素仍是裸 `P`**：调用方只转换类型恰为 `T` 的形参，容器元素无法逐个装箱。于是 `items[0]` 是裸对象，`return items[0]` 把它当成 `N` 交出去，任何 intf call 都读到非法 tag。
- `_build_obj_intf_upcast`（`irgen.c:271`）对泛型变量只发 `cast`，`cgen.c:1370` 把它降成 `move` —— 这对**形参**是正确的，对**容器元素**是错的。所以这不是「返回值类型检查缺失」，而是嵌套泛型形参的表示问题。

</details>

<details>
<summary><b>📊 边界对照表：7 种写法，修复前 vs 现在</b></summary>

2026-11-22 修复后复验，JIT 与 `KOALA_JIT=0` 结果一致。

| 写法 | 修复前 | 现在 |
|---|---|---|
| `func upcast[T:N](x T) N { return x }`（形参） | ✅ | ✅ 仍走 `cast`/`move` |
| `func opt_upcast[T:N](x T) N? { return x }` | ✅ | ✅ |
| `func bound_to_trait[T:N](x T) N = x`（表达式体） | ✅ | ✅ |
| `let n N = P(); upcast(n)`（`T = N` 自身） | ✅ | ✅ |
| `func first[T:N](items list[T]) N { return items[0] }` | ❌ 断言 | ✅ **已修** |
| `func first[T:N](items list[T]) int { return items[0].k() }` | ❌ 断言 | ❌ 仍崩（`eval.c:470` / `vm_ops.h:547`） |
| `func fill[T:N](out list[N], items list[T]) { out.append(items[0]) }` 后外部调用 | ❌ 断言 | ❌ 仍崩（`eval.c:436`） |

后两行不经过返回值路径，正是 `docs/Koala_TODO.md` §8「泛型体内 T 的点号方法调用（itab 挂接）」作者决定搁置的场景，本次未触碰。

</details>

<details>
<summary><b>💥 影响</b></summary>

所有「泛型 + trait 约束 + 把容器元素交给 trait 类型的返回值」的写法此前都不可用。`koala/kparse` 的 `add_separated[T : Node](out list[Node], items list[T], seps)` 正是这个形状，导致任何带参数的函数在遍历 CST 时崩溃；当时已删除该函数、23 个调用点改为在各自非泛型 `slots()` 内联交织循环规避。

</details>

---

### 修复方案

<details>
<summary><b>✅ 已实施：返回路径按 trait 名在运行期挂 itab（2026-11-22）</b></summary>

新增 `OP_DYNAMIC_CAST rd, rs, <const-pool-idx>`，**只在 return 路径发射、只在 isel 降级**，irgen 字节不变。擦除版泛型体里具体类未知（一份 body 服务所有实例化），所以 itable 必须在运行期按 trait 名解析。

- **发射条件**（`isel.c` 既有 `type_is_generic_var(src_ts)` 分支）：目标是 trait + 源不是 param + **唯一 use 是 `ret`**（`isel_cast_sole_use_is_ret()`：`use_count==1`、`use_first`、`!is_def`、user 为 `OP_RET`）。多 use 的 cast 保持原状；返回形参跳过（调用方已装箱）。
- **编码（方案 A）**：`FORMAT_RRImm` = op8 | rd8 | rs8 | idx8，8-bit 常量池索引 + `ASSERT(<= UINT8_MAX)`，照抄 `is`/`as` 约定。`OP_WIDE` 是结构性保留位，不可用。
- **命名（方案 A）**：池里存**裸 trait 名**（`ts->klass_type.name`），与运行时 `IntfTable.name` 对齐 —— `write_klc.c:186` 写的是 `trait_sym->name`，`KlcIntfEntry`（`klc.h:171`）没有模块字段。IR dump 仍打印限定名。
- **opcode 编号稳定性**：新 op 追加在 `OP_LOAD_TYPE` 之后、IR-only 段之前，所有可执行 opcode 编号不变，21 个 git 跟踪的 `.klc` 不失配。
- **运行期查找**：共享 `kl_tp_find_itab(tp, name, len)`（`typeobj.c`）。`tp->itables` 含继承 trait，一次扫描同时覆盖父 trait 上转；`kl_typeof` 对接口值返回 `itab->tp`，已装箱的值也能用。池字符串**非 0 结尾**（`STR_BUF`/`STR_LEN`），故先比长度再 `memcmp`。
- **失败语义**：找不到 itab 时 `raise_exc_fmt("type '%s' does not implement trait '%.*s'")`，VM 走 `SAVE_PC(); goto error`，JIT 走 `cf->pc + trace_here` 后返回 0 跳错误 epilogue（仿 `kl_jit_shift_check`）。

`specialize.c` 与 cgen 的编码/打印通路未改（`FORMAT_RRImm` 已有）。JIT 的名字/格式表由 X-macro 自动生成。

</details>

<details>
<summary><b>📁 改动文件清单（10 个，+207/-1）</b></summary>

| 文件 | 内容 |
|---|---|
| `include/common/opcode_list.h` | 新 op + 编码文档 |
| `src/parser/klr/printer.c` | 并入 `print_ir_cast`，打印 `dynamic_cast %0 first:T to <module>.N`，不打索引 |
| `src/parser/backend/isel.c` | `isel_cast_sole_use_is_ret()` + 分支转换 |
| `src/parser/backend/lower_operands.c` | `lower_dynamic_cast_opers()`，`set_raw_const` 写池索引 |
| `include/runtime/object.h`、`src/objects/typeobj.c` | 共享 `kl_tp_find_itab()` |
| `include/runtime/vm_ops.h` | VM handler |
| `src/runtime/eval.c`、`include/jit/jit.h`、`src/jit/jit.c` | `kl_jit_dynamic_cast()` + JIT case（翻译期解析池字符串） |

</details>

<details>
<summary><b>🧪 验证</b></summary>

- 新增 `test/test-run/test_generic_return_cast.kl` PASS（`@first`/`@first_up` → `dynamic_cast`，`@passthrough` 形参源仍是 `cast`；运行时输出 `2/1/2`）。
- JIT 实测生效：`KOALA_JIT=1 KOALA_JIT_THRESHOLD=1` → `compiled 18, not compiled 0`，输出仍 `2/1/2`（加 handler 前是回退解释器，`jit.c` 对不支持的 opcode `goto unsupported`）。
- `./test-debug.sh`：633/633 lit + 9/9 C 单测；`./build-std-koala.sh`：266 PASS / 0 失败；编译 0 warning / 0 error。

</details>

<details>
<summary><b>⏸ 未实施（仍崩，见上方未勾选项）</b></summary>

泛型体内对容器元素做 trait 方法分派、以及 append 进 trait 容器。这两条不经过 return，需要 `docs/Koala_TODO.md` §8 的「使用点按 trait 名解析 itab」方案 —— 该方向原理上可行（本次 `kl_tp_find_itab` 已具备按名查找能力），但作者判定为时过早、明确搁置，重开前不推进。

</details>

<details>
<summary><b>⚠️ 已知限制（方案 A 的代价，非本次引入的回归）</b></summary>

1. **常量池 256 上限**：`dynamic_cast` 池索引只有 8 bit，而 `loadk` 是 16 bit。模块常量数 >256 且含泛型 return cast 时会在 `lower_operands` 断言失败。待补该场景回归。
2. **跨模块同名 trait 歧义**：运行时 itable 只有裸名，同名 trait 会取首个匹配。彻底消除需改 `.klc` 格式（给 `KlcIntfEntry` 加模块字段）。

</details>

<details>
<summary><b>🚫 曾否决的方向（重开时不必重提）</b></summary>

1. **编译期检查**：只看类型无法区分 `return x`（合法）与 `return items[0]`（崩溃），两者类型都是 `T`；按表达式来源区分规则过于复杂且会误禁 `let y = x; return y`。曾实现 `check_generic_invariant()`（只在 `parse_return` 检查 `T`/`T?`）并全量回归通过，但会误禁 4 种合法写法，**已全部撤回**。
2. **禁止 bound 为 trait 的 `T` 出现在嵌套形参类型里**：能堵住，但会禁掉 `func max_of[T:Comparable](xs list[T]) T` 这类常见写法，代价过大。
3. **调用点递归挂 bound[0] 的 itab**（TODO §8 已否决）：itab 需求归属泛型体内的使用点而非调用点，body 用第二个 bound 即失效；且 `get_intf_index` 按类查 lro，`T` 无类，索引无法烘焙进指令。

</details>

<details>
<summary><b>🩹 绕过办法（针对仍未修的两条）</b></summary>

把泛型辅助函数改成非泛型（形参直接写 trait 类型，调用方先构造 `list[Trait]`），或在非泛型上下文里内联循环。实测：无约束泛型 `func add[T](out list[N], items list[T])` 编译不过；显式局部上转 `let n N = items[i]` 在泛型内同样崩，故内联是唯一可靠规避。

</details>
