# `in` 重构后 `bytes` / `str` 的遗留项（待定 / 待实现）

← 返回 [问题列表](issues.summary.md)

- [ ] 进行中 —— API 取舍待定（48.1）；联合类型与字面量收窄仍有缺口（48.2）；`str` / `bytes` 3 处待修（48.3）；工具与产物 4 项待定（48.4）

> 来源：`docs/Koala_TODO.md` §48（2026-11-02，to decide / to implement）

---

### 详细描述

<details>
<summary><b>✅ 已落地（不是待办）</b></summary>

`bytes.__contains__` 现在接受 `uint8 | bytes`，`in` 把 `bytes` 操作数当子序列搜索读取（`src/objects/bytesobj.c:295`），所以不再需要单独的 `contains_bytes` 方法。

为了让那个联合形参可用，整型或浮点字面量现在会收窄到**第一个能容纳它的联合成员**（`narrow_lit_int`，`src/parser/parser_expr.c:178`）；没有整数成员能容纳的字面量只报一次错，报在第一个整数成员上（`300 in bs` → `Value 300 overflows 'uint8' range`）。规则记录在 `docs/Koala_Language_Manual.md:557`（Union Types）。

测试：`test/test-ut/test_bytes.kl`（`test_bytes_contains`、`test_bytes_contains_view`）与 `test/test-kl/test_bytes_in_range.kl`。

</details>

<details>
<summary><b>❓ 48.1 `bytes` API 面：待定（8 项）</b></summary>

1. **按空白切分。** `split(sep bytes, maxsplit = -1)` / `rsplit` 要求分隔符，所以 `b"a b  c".split()` 不存在。两种形态：单独的 `split_whitespace(maxsplit = -1)`（不改签名、不用做可选参数），或 `split(sep bytes? = nil, maxsplit = -1)`（更贴近 Python，需要 `nil` 默认值和 C 里的 `nil` 分支）。`str.split` 同样缺口 —— 它的默认是 `sep = " "`，所以 `"a  b".split()` 产出空片段而不是按空白串切分。
2. **`strip` / `lstrip` / `rstrip` 不接受字符集** —— 只有 `bytes_is_space`（`src/objects/bytesobj.c:809`：空格、`\t`、`\n`、`\r`）里固定的空白集。两件事要定：加 `chars bytes? = nil`，以及空白集是否应对齐 Python 的 `\t\n\v\f\r `（我们缺 `\v` 0x0B 和 `\f` 0x0C，所以 `b"a\x0c".strip()` 会保留换页符）。
3. **`splitlines()` 没有 `keepends`。** Python 默认与我们一致；`keepends = true` 缺失。
4. **`is_ascii()` 在 `bytes` 和 `str` 上都缺。** `str` 有 `isalpha` / `isdigit` / `isalnum` / `isspace` / `islower` / `isupper`；`bytes` 一个都没有（见第 7 项）。
5. **`count_bytes(sub, start, end)` 缺失。** `count(value uint8, ...)` 只数单字节，而 `index_bytes` / `rindex_bytes` 已经支持子序列搜索，于是子序列计数没有等价物：`b"aaaa".count_bytes(b"aa")`（Python：2，不重叠）被拒，报 `'count_bytes' is not found in class 'bytes'`（2026-11-02 实测）。
6. **`__eq__` / `__ne__` / `hash` 在 C 里实现并注册了，但没在 `bytes.kl` 声明**（`src/objects/bytesobj.c:991-997`）；且类头是 `bytes : Sequence[uint8]`，而 `str` 是 `str : Sequence & Comparable & ToString`。今天不出问题 —— trait bound 与方法签名都不检查（48.2 第 6 项），所以 `b"a" == b"a"`、`b"a" < b"b"`、`hashset[bytes]`、`dict[bytes, int]` 甚至用两个 `bytes` 调 `[T : Comparable]` 泛型都正确运行（2026-11-02 实测）。问题是**类头该不该说真话**：声明这三个 native 加 `& Comparable & Hashable`（并用 `Comparable` 取代手写的 `__lt__`…`__ge__` 块），还是记录下「`bytes` 只通过 native 暴露相等与排序，不声明这些 trait」。在有一致性检查之前，两者都只是文档 / 一致性决策。
7. **`str` → `bytes` 对齐。** 对比两者今天的 API（`bytes.kl` 与 `str.kl` 里的 `pub func` 名）：

   | | 名字 |
   | --- | --- |
   | `str` 有、`bytes` 缺 | `capitalize`、`center`、`ljust`、`rjust`、`zfill`、`lower`、`upper`、`isalpha`、`isdigit`、`isalnum`、`isspace`、`islower`、`isupper`、`remove_prefix`、`remove_suffix`、`substr`、`to_bytes`、`to_int`、`to_int_or`、`to_float`、`to_float_or`，以及 `__eq__` / `__ne__`（第 6 项） |
   | `bytes` 有、`str` 缺（有意为之） | `copy`、`copy_str`、`fill`、`zero`、`__setitem__`、`__setslice__`、`index_bytes`、`rindex_bytes` |

   两者都声明了 `__add__`、`__contains__`、`__getitem__`、`__getslice__`、`__lt__`…`__ge__`、`__init__`、`__mul__`、`count`、`empty`、`ends_with`、`fmt`、`has_next`、`index`、`iter`、`join`、`len`、`lstrip`、`next`、`next_strict`、`replace`、`reversed`、`rindex`、`rsplit`、`rstrip`、`split`、`splitlines`、`starts_with`、`strip`、`to_str`、`view`。

   这些到底哪些该上 `bytes`？字节数组是容器，所以 `substr` / `to_int` / `to_float` 可以不上 API，用 `to_str()` 作为桥（`str.to_bytes` 已经覆盖另一方向）；**看起来合理的增补**是字节级大小写折叠（`lower` / `upper`）与 `is*` 谓词，各自需要写清 ASCII-only 的定义（字节数组没有编码）。
8. **有意不做 —— 记录决策，避免重复讨论：** `translate` / `maketrans`、`partition` / `rpartition`、`expandtabs`、printf 风格 `%` 格式化，以及 `decode(encoding)`（只能经 `to_str()` 走 UTF-8，且非法输入 panic —— 不 panic 的 `to_str_or` / `validate` 是另一个缺口）。

   Hex 曾在此列表，2026-11-03 反转：`bytes.to_hex(sep = "", bytes_per_sep = 1)` 与静态 `bytes.from_hex(s)` 现在承载字节数组的十六进制文本（`sep` 是任意字符串，比 Python 的单字符更宽；`bytes_per_sep` 为正时从右分组，为负时从左；`from_hex` 跳过字节之间的 ASCII 空白，坏数字 panic 并带位置）。`koala/hex` 包已删除，`koala/crypto` 用 `to_hex()`；`koala/json/string.hex_byte` 仍手工格式化它的单个 `\u00XX` 字节。测试：`test/test-ut/test_bytes_hex.kl`。

---

### 修复方案

<details>
<summary><b>🧩 48.2 联合类型与字面量收窄：剩余缺口（6 项）</b></summary>

1. **联合类型只在形参位置被接受。** `var` 声明或类字段解析失败（2026-11-02 实测）：

   ```kl
   var v int | str = 0          // error: expected ';' or '='
   var v (int | str) = 0        // error: expected a type or '='
   class C { var v int | str }  // error: expected ';' or '=', then: expected field-decl or method-decl
   ```

   `docs/Koala_Language_Manual.md` 在 Union Types 下正好展示了 `var v int | str = 0` 这个例子，所以**手册跑在解析器前面**：要么让 var-decl / 字段的 type-spec 路径接受联合，要么把手册里的例子删掉。

2. **收窄只看字面量，这是设计选择。** `let i = 97; i in bs` 是编译错误（`int` 变量不会被隐式收窄，符合「不做隐式收窄」规则）；`uint8(i) in bs` 可用并截断运行期值，而常量越界会被拒（`uint8(300)` → `constant integer overflow in cast`）。`(i as uint8) in bs` 失败并报 `src of 'as' operator is not a valid type` —— 看着是**另一个 `as` 缺陷**，值得单独立条目。
3. **浮点字面量收窄不做范围检查。** 直接取第一个浮点成员（`src/parser/parser_expr.c:302-312`），所以若有 `float16` 成员会原样接受 `1e40`。今天 `std` 里没有这种成员，尚未出问题。
4. **多个整数成员：先到先得，无歧义诊断。** `func f(v uint8 | int)` 下 `f(7)` 拿到 `uint8`、`f(300)` 拿到 `int`（实测）。规则写在手册里；只有当这种静默选择被证明会造成困惑时才加警告。
5. **只有 `TYPE_INT` / `TYPE_FLOAT` 成员参与。** enum、alias 与 `bool` 成员永远不接收字面量，所以 `uint8 | bool` 这样的联合能让 `true` 落在 `bool` 成员上（正确），但永远不会把整数字面量收窄到 enum 成员。
6. **trait 一致性不检查方法签名，泛型 bound 也不检查。** 声明为 `Foo : Sequence[uint8]` 但把 `__contains__(item str) bool` 写错的类能编译并运行（实测：`"x" in f` 打印 `false`），而 `libs/std/builtin/collection.kl:55` 声明的是 `func __contains__(item T) bool`；同样 `func maxi[T : Comparable](a T, b T)` 接受两个 `bytes`，尽管 `bytes` 没有声明 `Comparable`（实测）。**所以 `bytes.__contains__(uint8 | bytes)` 这个放宽是被意外接受的，而不是靠逆变规则。** 如果将来收紧一致性检查，形参放宽（逆变、可靠）必须成为显式规则，否则 `bytes` 将不再符合 `Sequence[uint8]`。**同一个洞适用于 `std` 里的每个 trait 方法与 bound。**

</details>

<details>
<summary><b>🔧 48.3 `str` / `bytes` 里待修（3 项）</b></summary>

1. **`str.__mul__` 在非正计数上断言崩溃。** `"ab" * -1` 直接 abort 进程（`strobj.c:336: Assertion 'n >= 0' failed`），而它自己的文档写着 "Return an empty string if `n` is not positive"（`libs/std/builtin/str.kl:161`），且 `bytes.__mul__` 对 `n <= 0` 已经返回共享空数组。**修法**：去掉 `ASSERT`，返回空字符串，镜像 `_bytes_mul`。
2. **空白集比 Python 窄**（见 48.1 第 2 项）：`\v` 与 `\f` 不被 strip，而 `strip` / `lstrip` / `rstrip` 的文档注释枚举了同样的四个字符 —— 文档与 C 互相一致，但与 Python 不一致。
3. **`test/test-ut/test_bytes.kl` 的格式遗留。** 三处多余的双空行（约 387、635、782 行），`kl-fmt` 会删掉；另有 1180-1196 的注释块（见 48.4 第 1 项）。

</details>

<details>
<summary><b>🛠 48.4 工具、产物与提交范围（4 项）</b></summary>

1. **`kl-fmt` 会重写注释里的代码。** `test/test-ut/test_bytes.kl:1180-1196` 原样保留编译器诊断作为文档（`// print(uint8(300)) // error: constant integer overflow in cast: cannot cast from int64 to uint8`）；`python3 tools/kl-fmt.py --diff` 想把那些行按 100 列折行并重新缩进。在这个问题定案前，该文件**永远无法 `--check` 干净**。决定：跳过只有 `//` 的块、加 `// kl-fmt: off` / `on` 指令，或接受重排。
2. **`dist/std/*.klc` 是编译器会加载的构建产物。** 每次改 `.kl` 都要重跑 `libs/std/build-and-test.sh`（或 `wombat`），并把重新生成的 `libs/std/dist/std/builtin.klc` 与源码一起提交 —— 否则全新检出会用过期字节码编译。待定：是否总是把重新生成的 `.klc` 包含进提交（**推荐**，因为编译器读字节码），以及是否要加一项 CI 检查来验证提交的 `.klc` 与全新构建一致。
3. **`libs/std/dist/std/fs.klc` 每次构建都会被重写，尽管源码没动** —— 大小相同、字节不同；2026-11-02 已经被 `git checkout` 回滚过多次。找出不确定性来源（时间戳 / 序列化器里的哈希表遍历顺序？），或在 git 里忽略该文件。
4. **当前工作树的提交拆分**（当时尚未提交）。建议：(a) `bytes` / `str` 的运算符与方法批次 —— `__mul__`、`__lt__`…`__ge__`、`join`、`replace(count)`、空 `bytes` 单例、`test_empty_bytes_stable`，以及 `str.join` 重构（`strobj.c`、`str.kl`、`test_str.kl`）；(b) `bytes.__contains__(uint8 | bytes)` + 联合字面量收窄（`parser_expr.c`）+ 手册 + 测试；(c) `libs/koala/http/response.kl`（及其 `.klc`）。还需决定：`libs/koala/dist/**/*.so`（`libkoala_crypto.so`、`libkoala_tls.so`）这类预编译二进制到底该不该进仓库。

</details>

<details>
<summary><b>🔗 48.5 交叉引用</b></summary>

- 可变 `bytes` 字面量共享 const pool：[#73](issue-73.md)（§37 Open 第 1 项，`docs/Koala_TODO.md:2072`）—— 仍开着，本次重构未改变它。
- `+` 在 `bytes` / `str` 上被当作可交换：[#117](issue-117.md)（§47）—— 2026-11-02 已修（47.2）；`lsra` 兜底（47.3）仍待议。
- 单类型参数泛型**类**的 bound 会崩编译器：[#106](issue-106.md)（§16.1b，`Heap[T : Comparable]`、`SortedSet[T : Comparable]` …）。**自由泛型函数不受影响**（`maxi[T : Comparable]` 可用），所以不阻碍 48.1 第 6 项；但它阻碍 `sortedset` / `heapq` 家族使用 `Comparable` 的 `bytes`。
- `is` / `as` 遗留：[#102](issue-102.md)（§29）—— 48.2 第 2 项的 `i as uint8` 失败见那里。

</details>
</details>