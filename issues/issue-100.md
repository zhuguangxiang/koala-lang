# `koala/redis` 遗留项：等待 mock 注解的测试、TLS 等

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 6 项待 mock 测试 + 5 项其它遗留；TLS 部分已修

> 来源：`docs/Koala_TODO.md` §62（2026-11-06，to do）

---

### 详细描述

<details>
<summary><b>🧪 等待 mock 注解的测试</b></summary>

这些路径需要一个**能按需失败**的 socket。

- `Conn.receive`：对端 reset 给出 `ERR_IO`，读超时给出 `ERR_TIMEOUT`，两者都把 Conn 标记为 broken。
- 往已关闭的连接写返回短计数且**不** panic（`TcpConn.write`）。
- `Pipeline`：线级失败之后不再读，剩余命令共享那个失败的 Result。
- `SingleTcpPool`：坏掉的 Conn 被丢弃，下次 acquire 重连。
- `Conn.setup`：`Options.name` 被拒时关闭 Conn，acquire 返回 nil。
- `TcpConn.read` 在超时时返回 -2。VM 把 `SO_RCVTIMEO` 映射成 `IO_TIMEDOUT` 还是 `IO_WOULDBLOCK` **尚未核实**。

对真实服务器的测试先检查其在线，不在线时直接返回而不 panic。

</details>

<details>
<summary><b>✅ TLS（2026-11-08 已修）</b></summary>

- ~~`SingleTlsPool` 没有 `_in_use` 标志~~：现在像 `SingleTcpPool` 一样独占借出（`test_tls_pool_lends_exclusively`）。
- ~~`tls.dial` 不接 options~~：`Options.connect_timeout_ms` 同时约束 TCP 连接（`so.DialOptions`）与 TLS 握手（`tls.TlsOptions`）。
- `connect_tls` 与 `SingleTlsPool` 曾对着本地一个仅 TLS 的 redis（用 `SSL_CERT_FILE` 指一个一次性 CA）手工跑通：ping / set / get 按名字与按 IP 都正常。**仍无自动化测试**，见 [#99](issue-99.md)。

</details>

---

### 修复方案

<details>
<summary><b>🛠 其它遗留</b></summary>

- **写失败看不到**：`BufWriter` 忽略短写，所以 `Conn` 只能从后续的读或读超时才发现。旧客户端会检查 `io.write_all`。
- **非 UTF-8 的 bulk 值在 `decode` 里 panic**（`bytes.to_str`），于是二进制值会**杀掉程序**，且 API 没有 `bytes` 形式。
- **数组回复里的服务端错误只保留第一个**，因为 `Reply` 没有错误类型。这对 `EXEC` 有影响；与事务工作一起决定。
- **不支持**：`Options` 里的 `AUTH` / `SELECT`（否则重连后会丢）、事务、Pub/Sub、RESP3、Unix socket、连接 URI、`SCAN`。
- **`redis` 不在 `libs/koala/__ws__.toml`** 里，所以 `wombat build koala` / `wombat test koala` 会跳过它。
- **`libs/koala/redis_old` 可以删掉**：没有任何引用，git 里有旧文件。

</details>