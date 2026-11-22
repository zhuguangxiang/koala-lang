# list 接口全面核对：声明-实现-trait 三方不一致 + 主流语言对齐

← 返回 [问题列表](issues.summary.md)

- [x] 主体已修复 —— 2026-11-22 复测：原 11 个 not_impl 方法与 4 个编译失败/崩溃项**全部可用**，两个 for-in bug 也已消失
- [ ] 仍开：`sort_by` / `key=`、命名与语义收敛（P2）、**编译器不强制 trait 一致性**（设计决策）

> 来源：`docs/Koala_TODO.md` §18（2026-10-03 建立，含 2026-10-28 / 11-08 更新）
>
> 审计方法：对 `libs/std/builtin/list.kl` 声明的 **23 个 `@native` 方法**逐一写单文件探针编译 + 运行实测，并与 C 侧 `src/objects/listobj.c` 的 `list_methods[]` 交叉核对。

---

### 详细描述

<details>
<summary><b>✅ 2026-11-22 复测（本页最重要的一条）</b></summary>

用单个探针程序把 §18.1「not_impl 11 个」与「编译报错 / 崩溃 4 个」逐项跑了一遍：

```kl
var l = [3, 1, 2]
print(l.empty(), l.count(1), l.index(2))   // false 1 2
l.insert(0, 9); print(l)                   // [9, 3, 1, 2]
l.remove(9); l.reverse(); print(l)         // [2, 1, 3]
print(l.reversed(1).next())                // 3
let c = l.copy(); print(c, c.len())        // [2, 1, 3] 3
print(l + [7])                             // [2, 1, 3, 7]
l += [8]; print(l)                         // [2, 1, 3, 8]
l[0:1] = [5]; print(l)                     // [5, 1, 3, 8]
l.clear(); print(l.len())                  // 0
print(l.iter(1).next())                    // nil
```

结论：**`empty` / `index` / `count` / `insert` / `remove` / `clear` / `reverse` / `reversed` / `copy` / `iter` / `__setslice__` / `__add__` / `__iadd__` 全部已可用**，`iter(step)` 签名也已对齐 `Iterable`。`rindex` 已从 `list.kl` 移除（`Sequence` / `MutableSequence` 并不要求它），因此不再是欠账。

`list_methods[]`（`src/objects/listobj.c:830`）现在注册了 23 项：`__init__`、`len`、`empty`、`__getitem__`、`__setitem__`、`__contains__`、`index`、`count`、`__getslice__`、`__setslice__`、`append`、`pop`、`copy`、`fmt`、`extend`、`insert`、`remove`、`delete`、`clear`、`reverse`、`sort`、`__add__`、`__iadd__`。

两个 for-in bug（18.3）也复测通过：

```kl
// Bug A：顶层 for-in 后收尾越界
let l = list[int](10, 20, 30)
for v in l { print(v) }
print("done")              // 10 20 30 done，exit 0（原为 Error: list index out of range）

// Bug B：inline iterable
func main() { for v in list[int](1, 2) { print(v) } }   // 1 2，exit 0（原为 irgen.c:2355 UNREACHABLE）
```

`sort` 也已实现（`sort(reverse = false)`，稳定归并排序，2026-11-08）。

</details>

<details>
<summary><b>📜 历史快照（2026-10-03，已过时，保留备查）</b></summary>

**当时可用（8）**：`__init__` / `len` / `__getitem__` / `__setitem__` / `__getslice__`（`l[a:b]`）/ `append` / `pop` / `extend`。另有 C 侧实现但 `list.kl` 未声明的 `__str__`（`print(l)` 输出 `[1, 2, 3]`）。

**声明 `@native` 但无 C 实现（运行时命中 not_impl 哨兵，11 个）**：`empty`、`index(v, start, end)`、`rindex(v, start, end)`、`count(v, start, end)`、`insert(i, v)`、`remove(v)`、`clear()`、`reverse()`、`reversed()`、`copy(start, end)`（含带范围形式）、`iter()`（直接调用 `l.iter()` 编译报 `'iter' is not found in instance of class 'list'` —— for-in 走内建字节码协议，声明的 `@native iter()` 无实体；且缺 `step` 形参）。

**编译报错 / 崩溃（4 个）**：

| 方法 / 语法 | 当时现象 |
|---|---|
| `__contains__`（`v in l`） | ~~编译报 `type 'list<j>' does not support '__contains__' method`~~ **Resolved (2026-10-28)**：`list.kl:46` 已声明，`v in l` 可用，`test-ut/test_ut_list.kl` 的 `test_list_contains` 覆盖 |
| `__setslice__`（`l[0:2] = xs`） | 编译报 `cannot resolve expr's type`（对应 §12 `OP_SEQ_SET_SLICE` 未实现） |
| `__add__`（`l + xs`） | **崩溃**：`vm.c:50 kl_not_impl_func` 断言失败（`Assertion '(obj)->_type == &cfunc_type' failed`），进程直接 abort，比 not_impl 异常更严重 |
| `__iadd__`（`l += xs`） | 编译报 `inplace assignment is not supported for 'list' type` |

**trait 一致性破损（当时）**：`MutableSequence` 要求的 `insert` / `remove` / `clear` 未实现，list 实际上不满足它声明实现的 `MutableSequence`；`Iterable` 要求 `iter(step = 1)`，list 声明无参 `iter()`；`Sequence.__contains__` / `index` / `count` 三件全缺。

**for-in 两个 bug（当时）**：Bug A = 模块顶层（`__init__` 中）对 list / str / bytes 的 for-in，循环体正常但 `__init__` 收尾多做一次越界访问（`Error: list index out of range`），普通函数内与顶层 `range` 都不受影响；Bug B = `for v in list[int](1, 2)` 直接 `irgen.c:2355: Why goes here?`（UNREACHABLE），因为 for-each 分派只处理 `klr_is_const(it_val)` 下的 range / seq 两类常量，构造调用等一般表达式落入 `UNREACHABLE()`。

</details>

<details>
<summary><b>🌍 与 Python / Rust / Go 的对齐分析（18.4）</b></summary>

参照系：

- **Python `list`**：append / extend / insert / remove / pop / clear / index / count / **sort** / reverse / copy，加 `len()` / `in` / 切片 / `+` / `+=`。
- **Rust `Vec`**：push / pop / insert / remove / clear / len / is_empty / contains / **sort / sort_by / dedup / truncate / retain / resize** / reverse / extend / first / last / position。
- **Go `slices` 包**：Append / Insert / Delete / Contains / Index / **Sort** / Reverse / Clone / Equal。

**缺少（建议补）**：

1. `sort_by(cmp)` / `key=` —— `sort()`（含 `reverse = true`）已实现，但**比较键仍缺**；
2. `append` 与 `push` 的统一（见 18.2 第 1 条，现已统一为 `append`）；
3. `first()` / `last()` —— Rust 风格便捷访问，可选；
4. `truncate` / `resize` / `retain` / `dedup` / `position` —— Rust 风格，可用推导式替代，**优先级低**。

**冗余 / 值得商榷**：

1. `copy(start, end)` 与 `__getslice__`（`l[a:b]`）功能重叠 —— Python 的 `copy()` 是无参整体浅拷贝；**建议 `copy()` 收敛为无参整体拷贝，范围复制交给切片语法**；
2. `empty()` 等价于 `len() == 0`，**命名建议对齐 Rust `is_empty()` / Go 惯例**。

**合理应保留**：push / pop / extend / insert / remove / clear / index / rindex / count / reverse / reversed / len / 切片 / `+` / `+=` —— 方向与主流一致，前提是真正实现。

</details>

---

### 修复方案

<details>
<summary><b>🧩 trait 一致性（18.2，现状已基本对齐）</b></summary>

1. **`append` 接口一致性**（§16.3 已定名）：`MutableSequence`（`collection.kl`）要求 `append(value T)`，list 已使用 `append`；当时缺 `insert` / `remove` / `clear`，**现已全部实现**，list 满足其声明实现的 `MutableSequence`。
2. **`iter` 签名**：`Iterable` 要求 `iter(step = 1)`，list 现在是 `pub func iter(step = 1) Iterator[T] = ListIter[T](self, step)`（`list.kl:26`），**已对齐**，`l.iter(1).next()` 可用。
3. **`Sequence.__contains__` / `index` / `count`**：三件均已实现（`listobj.c:836-838`）。
4. **编译器不强制 trait 一致性（仍开，属设计决策）**：`.kl` 中声明存在即可通过编译（即使 `@native` 无 C 实体、trait 方法整体缺席），所有问题推迟到运行时暴露。是否加编译期一致性检查需另行拍板 —— 与 [#118](issue-118.md)（§48.2 第 6 项）是同一个洞：现在连**签名**都不检查。

</details>

<details>
<summary><b>🎯 施工优先级（18.5，按当前状态更新）</b></summary>

- ~~**P0（崩溃 / 核心功能不可用）**~~ **全部完成**：`__add__` 断言崩溃已修（`listobj.c:852` 的 `_list_add`）；for-in 两个 bug 已消失；`__contains__` 已实现，`in` 运算符对 list 可用。
- ~~**P1（trait 契约兑现）**~~ **全部完成**：11 个 not_impl 方法、`__setslice__`、`__iadd__`、`append` 定案、`iter(step)` 签名。
- **P2（能力补齐，仍开）**：
  - `sort` ✅（`sort(reverse = false)`，2026-11-08，稳定归并）/ **`sort_by(comparator)` 或 `key=` 待做**；
  - **`copy()` 语义收敛**：`copy(start = 0, end = -1)` 与切片重叠，建议收敛为无参整体浅拷贝；
  - **`empty` → `is_empty`** 命名；
  - `first` / `last`（可选）；`truncate` / `resize` / `retain` / `dedup` / `position`（低优先级）。

</details>

<details>
<summary><b>🧪 测试现状与挂点（18.6）</b></summary>

- 现有用例只覆盖可用面：`test/test-run/test_list*.kl`（5 个文件）+ `test/test-kl/test_list.kl` 使用 `push` / `len` / 下标赋值 / 切片。test-ut 侧已有 `test/test-ut/test_list_sort.kl`（10 个用例：`sort` 的空 / 单元素 / 升降序 / 稳定性 / 前部 pop 后排序 / 不可比 panic）。
- 每实现一个欠账方法，应在 `test/test-ut/test_list*.kl` 补用例，参照 `test/test-ut/test_bytes.kl` 模式（`@test` + `ut.assert_*`，含范围语义与视图 / 切片交互）。
- 本页 2026-11-22 的复测探针建议固化为用例：`empty` / `count` / `index` / `insert` / `remove` / `reverse` / `reversed(step)` / `copy` / `__add__` / `__iadd__` / `__setslice__` / `clear`，以及两个 for-in 形态（顶层 + inline iterable），后者同时是 §18.3 的回归护栏。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#106](issue-106.md)（§16）：声明-实现欠账清单（`@native` 无 C 实现），与本节的审计方法同源；§16.3 即 `append` 定名。
- [#118](issue-118.md)（§48）：`bytes` / `str` 的 API 对齐清单，trait 一致性同一个洞（48.2 第 6 项）。
- [#54](issue-54.md)（§11）：`OP_SEQ_GET_IMM` / `OP_SEQ_SET_IMM` 负下标语义，影响 `index` / 切片的边界定义。
- `test/test-ut/test_list_sort.kl` 与 `sort` 的实现见 §38 第 7 条。

</details>