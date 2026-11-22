# `BufReader.read_line` 保留 CRLF 中的 `\r`

← 返回 [问题列表](issues.summary.md)

- [ ] 待决策 —— 方案已写但**未实施**（原记录标注 "not applied, open to adjustment"）

> 来源：`docs/Koala_TODO.md` §52（2026-11-03，to decide）

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`read_line`（`libs/std/io/bufio.kl`）就是 `read_until(out, '\n')`：`\n` 被消费且不 append，但它前面的 `\r` 被保留，所以 `"a\r\nb"` 读出 `"a\r"`。

`Lines` 两个都剥（`libs/std/io/splitter.kl`），于是**行迭代与 `read_line` 行为不一致**；每个 CRLF 协议都得手工剥（`koala/redis/protocol.kl` 做 `line.to_str().remove_suffix("\r")`）。

`test/test-ut/test_read_line_byte.kl`（`test_read_line_keeps_cr` 及其头注释）**固化了当前行为**。

</details>

<details>
<summary><b>🌍 其它语言的对照</b></summary>

- **丢 `\n` 的调用也丢 `\r`**：Go `bufio.Reader.ReadLine` 与 `Scanner`、Java `BufferedReader.readLine`、C# `StreamReader.ReadLine`、Rust `lines()`。
- **保留 `\n` 的调用保留 `\r`**：Go `ReadString('\n')`、Rust `read_line`、Python 二进制 `readline`。

Koala 的 `read_line` 恰好卡在**两者之间**。

</details>

---

### 修复方案

<details>
<summary><b>📐 语义</b></summary>

- 丢掉紧邻 `\n` 之前的那个 `\r`；**行内的 `\r` 保留**（孤立 `\r` 不算行尾）。
- 最后一行没有 `\n` 时，丢掉其末尾的 `\r`（同 Go 的 `Scanner`）。
- 返回的计数**不含**被丢掉的 `\r`。
- `read_until`、`Lines`、`Splitter` 不改；原始字节仍可经 `read_until(out, '\n')` 取得。

</details>

<details>
<summary><b>🔧 实现（只改 `bufio.kl`）</b></summary>

`read_line` 走自己的循环，用一个局部的「暂扣 `\r`」标志 —— 因为 `\r` 可能结束一个 chunk、`\n` 开下一个 chunk。

- 某个不含 `\n` 的 chunk 以 `\r` 结尾时：该字节不写出，置标志。
- 下个 chunk 若以行尾开头则丢掉它，否则先写出它。
- EOF 时丢掉。

**为什么要用标志而不是把 `\r` 留在读缓冲区**：capacity 为 1 时，暂扣的字节会占满缓冲区，`fill()` 再也读不进东西。不需要 `Buffer.truncate`。

参数名从 `_buf` 改回 `out`，与 `read_until` 一致。

</details>

<details>
<summary><b>📚 文档</b></summary>

`read_line` 的 doc 注释与 `docs/std_io_api.md` 中的对应条目。

</details>

<details>
<summary><b>🧪 测试</b></summary>

- 修改既有用例 `test_read_line_keeps_cr`，期望改为 `1` / `"a"`，并修正其头注释（**既有测试，需先达成一致**）。
- 新增：`\r` 与 `\n` 分处两个 chunk（capacity 2）、行内的 `\r`、暂扣的 `\r` 后面紧跟另一个字节、空的 CRLF 行、EOF 处无 `\n` 的 `\r`、capacity 1 配 CRLF 行、CRLF 行与 `read_byte` 交错。

</details>

<details>
<summary><b>❓ 未决点</b></summary>

- 孤立的 `\r` 是否也应结束一行（Java 会）。
- 是否加一个保留行尾符的变体。

</details>