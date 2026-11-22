# `koala/http` 遗留项

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 5 项待做

> 来源：`docs/Koala_TODO.md` §80（2026-11-08，to do）

---

### 详细描述

<details>
<summary><b>1️⃣ HEAD 请求</b></summary>

native 侧 `http_parser_skip_body` 已存在，但客户端从不调用它，`Request` 也只有 `get` / `post`。

手工构造一个响应带 `Content-Length` 的 `Request("HEAD", ...)` 会**等到 EOF**，然后返回 nil。

</details>

<details>
<summary><b>2️⃣ 1xx 响应（待核实）</b></summary>

llhttp 会把 `100 Continue` 当作一条独立消息完成，而客户端在第一条完整消息处停止，所以它可能把 100 当成响应返回。**未验证。**

</details>

<details>
<summary><b>3️⃣ Keep-alive</b></summary>

客户端强制 `Connection: close`。parser 其实已有 `keep_alive` 与 `reset`。

**决定**：在 `koala/http` 内部保留连接池，等到出现第二个真实的池再决定是否抽 `koala/pool`（2026-11-08 讨论：共享内核很薄，「何时可复用连接」由各协议自己决定，且没有并发 API 时一个池比「每个 host 复用一条连接」强不了多少）。

</details>

<details>
<summary><b>4️⃣ 解析错误不上报</b></summary>

`http_parser_error` 未被使用，`Response.parse` 直接返回 nil 而不给原因。

</details>

<details>
<summary><b>5️⃣ 请求解析（面向服务端）</b></summary>

parser 能接受请求，但 Koala 侧还没有任何东西用它。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办汇总</b></summary>

原记录按问题逐条列出，未给统一方案。落点分别是：

- HEAD：客户端接上 `http_parser_skip_body`；`Request` 增加 `head`（或允许构造任意方法）。
- 1xx：先验证，再在客户端决定跳到下一条消息。
- keep-alive：在包内实现连接池。
- 诊断：把 `http_parser_error` 接进 `Response.parse` 的失败路径（与 [#64](issue-64.md) 第 2 项同类：缺诊断）。

</details>