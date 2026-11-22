# 小的编译器与运行期问题（7 项）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 7 项待修或待定

> 来源：`docs/Koala_TODO.md` §65（2026-11-06，to fix or decide）

---

### 详细描述

<details>
<summary><b>1️⃣ 空 list 字面量不从形参取类型</b></summary>

`encode("PING", [])`，形参为 `args list[str]`，报 `expected 'std/builtin.list<u>', but got 'std/builtin.list<o>'`。

（与 [#66](issue-66.md)、[#73](issue-73.md) 第 4 项同族。）

</details>

<details>
<summary><b>2️⃣ `nil` 的类型在错误信息里显示为 `_?`</b></summary>

`lhs: any =/= rhs: _?`。

</details>

<details>
<summary><b>3️⃣ `koala -c <dir> -o <file>` 忽略 `-o`</b></summary>

选项跟在目录后面时 `-o` 被忽略，klc 写在目录旁；`koala -c -o <file> <dir>` 才正常。

</details>

<details>
<summary><b>4️⃣ `io.Reader` 的契约与实现不符</b></summary>

`io.Reader` 写着 "Never return a negative value"，但 `TcpConn.read` 返回 -1 / -2，`TlsConn.read` 也返回负值。`BufReader` 与 `io.copy` 能处理，只是**契约文字没写**。

</details>

<details>
<summary><b>5️⃣ 写失败的报告方式不统一</b></summary>

`TcpConn.write` 用短计数报告失败，而 UDP / raw socket 在 I/O 失败时仍然 panic。

</details>

<details>
<summary><b>6️⃣ 非 wombat 工程下 native 库只能靠 `LD_LIBRARY_PATH`</b></summary>

脚本 import `koala/tls` 会打印 `Failed to load libkoala_tls.so`（需先 `source env.sh`）。在 wombat 下已被处理。

曾尝试从 klc 所在目录加载，**已回退**：路径归属 wombat。

</details>

<details>
<summary><b>7️⃣ `is` 检查的窄化变量缺索引测试</b></summary>

`test-run/test_narrow_index.kl` 只覆盖了 optional 窄化。

</details>

---

### 修复方案

<details>
<summary><b>🛠 原记录未逐项给出方案</b></summary>

原记录只列出问题。可直接着手的方向：

- 第 1 / 2 项：空字面量的期望类型传递 + `nil` 类型显示串修正（诊断层）。
- 第 3 项：`koala` 参数解析改成不依赖选项与位置参数的先后。
- 第 4 / 5 项：统一 `io.Reader` / `Writer` 的负值与短计数契约（先改文档，再统一 socket 侧行为）。
- 第 6 项：维持现状，在文档中写明 `source env.sh` 的前提。
- 第 7 项：补 `is` 窄化后的索引测试。

</details>