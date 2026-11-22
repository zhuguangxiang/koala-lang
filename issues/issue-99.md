# `koala/tls` 遗留项

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— **无自动化测试**；5 类功能不支持

> 来源：`docs/Koala_TODO.md` §81（2026-11-08，to do）

---

### 详细描述

<details>
<summary><b>✅ 已修（2026-11-08）</b></summary>

- 证书对主机名校验（`SSL_set1_host`）；IP 字面量则对其 IP 条目校验且不发 SNI。
- 没有 `close_notify` 的关闭视为 EOF（`SSL_OP_IGNORE_UNEXPECTED_EOF`）。
- `close()` 调两次是安全的。
- 握手失败会关闭 TCP 连接。
- 新增 `dial(url, options?, net_options?)` 与 `TlsOptions`（握手超时、`server_name`、`skip_verify`）。

</details>

---

### 修复方案

<details>
<summary><b>🧪 测试缺口（No automated test）</b></summary>

- **IP 字面量路径**：只对着本地一个证书含 IP 条目的 TLS redis 手工验证过。本机到 `1.1.1.1` / `8.8.8.8` 的 443 不可达，所以对公网服务器的测试会挂住。
- **Unexpected EOF**：没有公网服务器会可靠地不发 `close_notify` 就关闭，所以 `test_tls_read_to_end` 复现不出旧故障。
- `redis.connect_tls` / `SingleTlsPool` 对真实服务器（仅手工，§62）。
- 网络测试用 example.com 与 badssl.com，离线时提前返回。

</details>

<details>
<summary><b>🚫 不支持</b></summary>

- `ca_file`（自定义根 CA）：目前只有一个进程级 `SSL_CTX`，需要「每个配置一个 `SSL_CTX`」。
- 最低协议版本、ALPN、客户端证书。
- 服务端（`TlsListener`、服务端握手）。
- 连接状态：协商出的版本、cipher、对端证书。
- 失败原因：`dial` 与 `wrap` 对 DNS、TCP、握手、校验失败**一律返回 nil**，无法区分。

</details>

<details>
<summary><b>⚠️ 需要知道的行为</b></summary>

设了握手超时后，`wrap` 事后会**清掉** TCP 连接的读、写超时。这也会清掉 `so.dial` 为连接超时留下的 `SO_SNDTIMEO`。

</details>