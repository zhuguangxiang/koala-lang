# `koala/re` 与 Python `re` 的未对齐项

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— API 已于 2026-11-01 对齐；余 8 项缺失 + 12 项行为差异 + 3 项后续

> 来源：`docs/Koala_TODO.md` §41（2026-11-01，to do / to discuss）

---

### 详细描述

<details>
<summary><b>✅ 已对齐（对照 Python 3.14）</b></summary>

`libs/koala/re` 的 API 跟随 Python 的 `re`；除有意保留的 `SEARCH` / `MATCH` / `FULL_MATCH` 配 `Pattern.exec` 与公开构造器外，没有 Koala-only 或 Go-only 的 API 残留。

2026-11-01 对齐：`compile` / `search` / `match` / `fullmatch` / `findall` / `finditer` / `sub` / `split` / `escape` / `purge`；flag `NOFLAG` / `I` / `IGNORECASE` / `M` / `MULTILINE` / `S` / `DOTALL` / `A` / `ASCII` 取值同 Python；`Pattern.pattern` / `flags` / `groups` / `groupindex` 及其方法（带 `pos` / `endpos`）；`Match.re` / `string` / `regs` / `pos` / `endpos` / `group` / `start` / `end` / `span`（按编号或名字）/ `groups` / `groupdict` / `expand`；Python 的替换模板（`\1`、`\g<name>`、`\0`、八进制、`\n` …）与错误；Python 的 `count`；`split` 插入分组；`Pattern` 与 `Match` 的打印形式。

第二轮也于同日对齐：`subn`；`findall` 单分组时给分组 1；`\d \w \s \b` 默认 Unicode，`ASCII` 时仅 ASCII，且 `IGNORECASE` 只折叠 ASCII 字母（`\d \w \s` 从不做 case folding，同 Python）；`\Z`、`\uXXXX`、`\UXXXXXXXX`、`(?a)`、`(?u)`；以及 Python（3.7+）的空匹配规则，用于 `findall` / `finditer` / `sub` / `split`，`split` 的切分片段同 Python。

同时移除：`UNGREEDY`、`is_match`、`replace`（现为 `sub`）、`replace_literal`、`replace_with` / `Replacer`、`Pattern.group_name` / `group_index` / `group_names`、`Match.regex()` / `input()` / `indexes()`（现为 `re` / `string` / `regs`）/ `matched` / `named` / `submatches`，以及 `findall` 的 `n` 上限。

</details>

<details>
<summary><b>🐍 Python 有、Koala 缺（本轮未加：该次重构不引入新 API）</b></summary>

1. ~~`subn`~~：2026-11-01 已完成。
2. `sub` 的 `repl` 用函数：Koala 还没有函数类型形参（见 [#103](issue-103.md) 第 5 项）。
3. `m[g]`（`__getitem__`）：被 [#110](issue-110.md) 挡住（用户类索引让 irgen abort）。
4. `group(1, 2)` 返回多个分组：需要变长参数与元组返回。
5. `Match.lastindex` / `lastgroup`：引擎得记录分组闭合的顺序；仅凭偏移无法区分 `((a))` 与 `(a)(b?)`。
6. `error` / `PatternError`（带 `msg`、`pattern`、`pos`、`lineno`、`colno`）、`RegexFlag`、`Scanner` 与 `Pattern.scanner`：错误现在是 panic，flag 是 int。
7. `Pattern.__eq__` / `__hash__`（pattern 与 flags 相等即相等）、`Pattern` 与 `Match` 上的 `__copy__` / `__deepcopy__`。
8. flag `U` / `UNICODE`、`L` / `LOCALE`、`X` / `VERBOSE`、`DEBUG`。

---

### 修复方案

<details>
<summary><b>⚠️ 行为未对齐（`41.2`，12 项）</b></summary>

1. **未参与匹配的分组给 `""` 而不是 `None`**：`group()`、`groups()`、`groupdict()` 以及 `split` 插入的分组都是如此。要 `None` 需要 `str?` 返回。
2. **`findall` 有 2 个及以上分组会 panic**（"use finditer"）：Python 那时返回元组，`list[str]` 装不下。0 或 1 个分组时与 Python 一致。
3. ~~空匹配跟随 Go~~：2026-11-01 已完成（C 迭代器现在有 Python 的 `must_advance`）。
4. **保留了 RE2 的语法扩展**：`\p{..}` / `\pL`、`\Q...\E`、`[[:alpha:]]`、`\x{...}` 与行内 `(?U)` 都被接受（Python 一个都没有）。编译错误用 Go 的措辞（"missing closing )"）。Python 的 `(?x)`、`(?L)` 以及「全局行内 flag 必须在前」的规则不支持。
5. **偏移是字节偏移**，模板与打印 span 的错误位置亦然；Python 是字符索引。保留：Koala 的 `str` 按字节索引。
6. **`Pattern.flags` 不含行内 flag**（如 `(?i)`），也不含 Python 默认的 `UNICODE`（32）。
7. **模板分组名**：任何非 ASCII 字符都算字母，Python 要求 `str.isidentifier()`。
8. **打印形式**：ASCII 之外的非可打印字符不转义（Python 写 `'\u200b'`）。
9. **关键字实参**：Koala 只允许「有默认值的形参」用关键字传，因此必传参数保留短名（`s`，不是 `string`）。
10. **Unicode 版本**：表是 Unicode 15.0（从 Go 生成），Python 3.14 是 16.0；16.0 新增的 5004 个 `\w` 与 80 个 `\d` 字符不匹配。逐码点核对过：在 15.0 的字符上，`\w \d \s \W \S \D`、字面量与字符类的 `(?i)`、以及 `ASCII` 变体都与 Python 一致 —— 除第 11 项。
11. **`(?i)` 与土耳其 i**：Python 的 `(?i)[a-z]` 也匹配 U+0130 `İ` 与 U+0131 `ı`，而 Unicode simple folding（我们用的）把它们分开。
12. **`$` 在无 `MULTILINE` 时**只在最末尾匹配；Python 也在末尾 `\n` 之前匹配（`re.search('a$', 'a\n')` 在 Python 里匹配，这里不匹配）。`\Z` 两者都是最末尾。

</details>

<details>
<summary><b>🐞 顺带遇到的编译器问题（`41.3`）</b></summary>

- `int | str` 持有 int 时 `x as str` 得到 nil，而静态类型是 `str`（编译器还警告其上的 `!` 是多余的）；`koala/re` 只用 `is` 窄化。与 [#102](issue-102.md)（§29）相关。
- 用户类索引：[#110](issue-110.md)（§42）。`<<=` / `>>=`：§43（已修）。空的元组列表：[#66](issue-66.md)（§44）。

</details>

<details>
<summary><b>➡️ 后续步骤（2026-11-01，按此顺序）</b></summary>

1. **`$` 改成 Python 语义**（41.2 第 12 项）：无 `MULTILINE` 时 `$` 也在末尾 `\n` 之前匹配。从文件读出的行保留 `\n`，所以 `re.search('x$', line)` 很常用而今天会**静默失败**。工作量小：parser（`re_core.c`）加一种断言类型，`\Z` 仍是绝对末尾；像其它断言一样对照 Python 验证。
2. **API 文档 `docs/koala_re_api.md`**，仿其它包文档的体例：API、语法（Python 的，去掉反向引用与 lookaround，加上 41.2 第 4 项的 RE2 扩展）、Unicode 与 `ASCII`、`sub` 的模板、字节偏移、线性时间保证，以及 41.2 的差异。
3. **`findall` / `finditer` / `sub` / `split` 的性能**：每次 `kl_re_iter_next` 都走 `kl_re_exec`，它会分配并释放 thread lists、栈与捕获缓冲；对长文本、多匹配的 find-all 循环要按匹配数付这份开销。应在循环间复用（例如放在 `ReIter` 里的 scratch 区，或由调用方传入）。

</details>
</details>