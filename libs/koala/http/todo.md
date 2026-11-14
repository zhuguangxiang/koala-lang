# koala/http 与 Go net/http 的对照

这份文档记录 koala/http 的 `Url` / `Request` / `Response` 与 Go `net/http`
的逐项对照，以及哪些差异是有意为之、哪些是待补的缺口。新增 `Client` /
`Transport` 层之前，先看这里——很多字段是那一层的前置。

## 分档

### 一档：已对齐

行为与 Go 一致，调用方按 Go 的直觉写就能工作。

| 能力 | koala/http | Go |
|---|---|---|
| scheme/host/port 拆分 | `Url.scheme()` / `host()` / `port()` | `URL.Scheme` / `Hostname()` / `Port()` |
| 默认端口 | `Url.parse` 按 scheme 填 80 / 443 | 同 |
| 请求报文格式 | `Request.to_bytes()` | `Request.Write` |
| 响应三件套 | `Response.status()` / `headers()` / `body()` | `Response.StatusCode` / `Header` / `Body` |
| 头名大小写不敏感查找 | `Response.header(name)` | `Header.Get` |
| Content-Length / chunked / close 三种 framing | `Response.parse` | `Transport` 内部处理 |
| 头结束位置 | `ResponseHead.body_offset()` | 无对应（Go 不暴露） |

`ResponseHead.body_offset()` 是 koala/http 比 Go 更细的地方：流式 client
需要“头在哪结束”来切分后续 read，Go 的 `Transport` 在内部做这件事，调用方
拿不到。这个设计要保留。

### 二档：有意简化

与 Go 不同，但当前场景下是更好的选择。改之前先想清楚代价。

| 项 | koala/http | Go | 简化的理由 |
|---|---|---|---|
| Header 存储 | `list[list[str]]`，有序 | `map[string][]string` | 序列化顺序确定，测试可精确断言报文；多值场景暂未出现 |
| Request.Body | `bytes` | `io.ReadCloser` | 阶段 3 读到 EOF 再 parse，body 已在内存；keep-alive 需要流式时再改 |
| Response.Body | `bytes`（已读完） | `io.ReadCloser`（流） | 同上；流式 client 用 `ResponseHead` + 手动 read 替代 |
| Status | `status() int` + `reason() str` 拆开 | `Status string`（`"200 OK"`） | 调用方判断 `status() == 200` 比解析 `"200 OK"` 方便 |
| Connection: close | 通过 `header("Connection", "close")` | `Request.Close bool` | 能工作，语义相同；专门的字段等需要时再加 |
| User-Agent | 通过 `header("User-Agent", ...)` | `Request.UserAgent` | 同上 |
| 协议版本 | 硬编码 HTTP/1.1 | `Proto` / `ProtoMajor` / `ProtoMinor` | 只做 1.1，不暴露版本字段 |

Header 用有序 list 是最大的分歧。改成 `map[string][]string` 会失去报文
顺序的确定性，测试里 `assert_eq(to_bytes(), "...")` 这种精确断言会变成
“包含某个头”的模糊断言。等 `Set-Cookie` 多值场景真正出现时，保留
`list[list[str]]` 存储，加一个 `header_all(name) list[str]` 访问器即可，
不改底层。

### 三档：真缺失

Go 有、koala/http 没有，且会影响上层功能。按优先级排。

| 缺失 | 影响 | 优先级 |
|---|---|---|
| `Url` 的 path/query 分开 | 现在 `path()` 返回 `"/search?q=koala"`，名字误导；用户拿不到 query 参数 | 高 |
| `Response.content_length()` | 调用方要知道“服务器声明了多少字节”，`body().len()` 只是“读到了多少” | 高 |
| `Response.transfer_encoding()` | 调试和重定向逻辑要知道 chunked | 中 |
| `Response.location()` | 重定向的前置 | 中 |
| `Response.cookies()` | `Set-Cookie` 解析 | 中 |
| `Request.content_length()` | 内部已算，调用方看不到 | 中 |
| `Request.Host` 覆盖 | 虚拟主机、`curl --resolve` 场景 | 低 |
| `Url.User`（`user:pass@host`） | Basic Auth 的 URL 形式 | 低 |
| `Url.Fragment`（`#frag`） | 客户端不发 fragment，但解析时不该丢 | 低 |
| `Url.Query()` → `map[str, str]` | query 参数解析 | 中（依赖 path/query 分开） |
| `Request.BasicAuth()` / `AddCookie()` / `FormValue()` | 认证和表单便利方法 | 低 |
| `Request.Proto` / `Close` 字段 | 协议版本和连接语义 | 低 |
| 请求侧 chunked | 流式上传 | 低 |

## 对齐顺序

按“`Client` / `Transport` 依赖什么”排：

1. **`Url` 拆 path/query** — `Request.to_bytes` 和将来的 `Url.Query()` 都依赖它
2. **`Response.content_length()` / `transfer_encoding()` / `location()`** — 重定向和 keep-alive 的前置
3. **`Header` 多值访问器** — 加 `header_all(name) list[str]`，不改存储
4. **`Request.content_length()`** — 内部已有，暴露即可

`Client` / `Transport` 层等 1–4 完成后建，否则那一层要反复改 `Request` /
`Response` 的字段。

## 不打算做的

| Go 有 | 不做 | 理由 |
|---|---|---|
| `Request.GetBody` | 重定向时自动重发 body | 第一版重定向不做；做时再议 |
| `Response.Request` | 从 response 反查 request | 调用方自己持有 request |
| `http.Pusher` / HTTP/2 | 只做 HTTP/1.1 | 范围外，明确不考虑 |
| `httptest.Server` | 测试服务器 | **已被下面落地的真 server 取代**：`Server.listen("127.0.0.1:0")` 就是测试服务器 |

---

# 服务端 / TLS（2026-11-14 落地）

## 已实现

| 能力 | 入口 | 说明 |
|---|---|---|
| 阻塞式 server | `Server.listen` / `serve` / `serve_once` / `accept` + `serve_conn` | 三个阻塞变体共用一条路径；连接内 keep-alive + 流水线 |
| 请求解析（服务端） | `Request.parse` | 复用同一个 `Request`，没有另造 ServerRequest；origin/absolute/`*` 三种 target |
| 响应序列化 | `Response.to_bytes` / `of` / `text` / `html` / `json` | reason phrase 表、Content-Length 自动 framing |
| TLS server | `Server.listen_tls` + `tls.Certificate` | `Certificate.from_pem` / `from_files` / `self_signed` / `cert_pem` |
| 流式响应 | `Response.stream` + `Body` trait / `FileBody` | 自动 chunked 分帧，source 每条路径都 close |
| 大小上限与超时 | `set_max_header_bytes` / `set_max_body_bytes` / `set_read_timeout` / `set_accept_timeout` / `set_handshake_timeout` | 431 / 413 / 408；空闲超时静默关闭 |
| 客户端 TLS 策略 | `Client(tls.TlsOptions())` | `set_skip_verify` / `set_ca_pem`（按连接覆盖信任库） |

测试：http 10 文件 132 用例；tls 本轮新增 26 用例（certificate 19 + server 7）。
JIT 开与 `--no-jit` 两种模式均全过。
真实 curl 验证过 HTTPS（`--cacert` 对 DNS/IP SAN 都 verify=0）、1MB 二进制 body、
SSE chunked 流、300KB 文件下载、流式后连接复用。

## 剩余待办

编号与任务列表一致。每条都带位置和依据，别只写"待补"。

### P0：影响可用性 / 安全

| # | 待办 | 位置与依据 |
|---|---|---|
| 16 | **并发模型**（协程 / fiber / 事件循环） | 语言层无线程无协程，`serve` 只能顺序 accept，一个慢客户端占住整个 server。这是下面多项的根因前置 |
| 42 | **请求侧流式 body** | `_read_request` 读完整个消息才交给 handler，大上传全量缓冲。响应侧已流式（见上），请求侧还没有；需要 `Body` 的镜像，以及 #21 也要用的先读头路径 |

### P1：常见场景

| # | 待办 | 位置与依据 |
|---|---|---|
| 20 | **ServeMux 路由** | 精确路径 + 尾 `/` 前缀匹配 + 404 兜底，作为 Handler 传入。**被 #23 阻塞**：`url().path()` 现在含 query，带参数的请求会匹配不上 |
| 21 | **`Expect: 100-continue` 的 interim 100** | 实测 curl 8.18 强制该头时回退延迟 1.001s 后才发 body，结果正确但慢。需要先读头再读 body 的路径 |
| 22 | **定长 body 的自动 chunked 编码** | 流式响应已自动分帧；剩下"定长 `bytes` + 调用方设了 `Transfer-Encoding`"这一种仍要自己 framing。见 `test_transfer_encoding_suppresses_content_length` |
| 23 | **`Url` 拆 path/query + 参数解析** | 即上文三档第 1 条，`path()` 返回 `"/search?q=1"`。含 percent-decoding |
| 24 | **handler 拿到 peer 地址** | `Request` 无 remote addr，做不了 IP 限流 / 访问日志。`serve_conn` 持有 `so.Conn` 但没传给 handler |
| 25 | **SNI 多证书** | 一个 Server 一个 Certificate，没设 `SSL_CTX_set_tlsext_servername_callback`，单 listener 服务多域名做不到 |
| 26 | **mTLS** | `native/certificate.c` 的 server CTX 没调 `SSL_CTX_set_verify`，不请求也不校验客户端证书 |

### P2：完善度

| # | 待办 | 位置与依据 |
|---|---|---|
| 27 | **Upgrade / WebSocket** | `Connection: upgrade` 被忽略，握手后的字节会被当 HTTP 解析。`llhttp_get_upgrade` 已在 vendored llhttp 里，shim 未暴露 |
| 28 | **证书内省** | `Certificate` 只有 `cert_pem()`，读不到 notBefore/notAfter、SAN、issuer，无法检查过期 |
| 29 | **自签密钥类型 / 长度可选** | `tls_cert_self_signed` 硬编码 `EVP_RSA_gen(2048)`，无 ECDSA |
| 30 | **ALPN** | server CTX 没设 `SSL_CTX_set_alpn_select_cb`，连 http/1.1 都不 advertise |
| 31 | **访问日志 / metrics 钩子** | 没有任何 per-request 记录（method/path/status/bytes/耗时） |
| 32 | **TLS session resumption 配置** | 全用 OpenSSL 默认，没有 cache 大小 / 超时 / tickets 开关的配置面 |

### 缺陷（已实现代码里的问题）

| # | 缺陷 | 位置与依据 |
|---|---|---|
| 33 | **`Connection` 头只整串比较** | `_keep_alive` 用 `v.lower() == "close"`，`"keep-alive, close"` 这类 token 列表解析不了（RFC 7230 §6.1） |
| 34 | **HTTP/1.1 缺 Host 不拒绝** | `_target_url` 放行空 authority。RFC 7230 §5.4 要求 400。当初为兼容 HTTP/1.0 故意放行，修它需要请求版本号，shim 未暴露 `llhttp_get_http_major/minor`。现有 `test_parse_no_host` 断言的是放行行为，要一起改 |
| 35 | **自签证书序列号恒为 1** | `ASN1_INTEGER_set(X509_get_serialNumber(x), 1)`。RFC 5280 要求同一 issuer 下唯一，严格客户端可能拒。应随机生成正整数 |
| 36 | **`cert_pem` 只导出叶证书** | 用 `SSL_CTX_get0_certificate`，拿不到中间链，`from_pem` 载入的链无法完整导出 |
| 37 | **加密私钥不支持** | `install_pem` 的 `PEM_read_bio_PrivateKey(kb, NULL, NULL, NULL)` 没传 passphrase 回调 |
| 38 | **`from_files` 超 64 KiB panic** | 用 `fs.read_str(path)` 默认 max=64KiB，大链会 panic 而不是载入 |
| 39 | **native 句柄泄漏** | `Certificate`(SSL_CTX) 与 `_Parser`(HttpParser) 都是裸 int，只有显式 `close()` 才释放，无 finalizer。沿用既有模式，修它可能要语言层支持 |
| 40 | **handler panic 带走进程** | `serve_conn` 调 `handler(r)` 无隔离。**尚未实测**，先确认行为再决定是 per-request 恢复成 500 还是文档约定 handler 不得 panic |
| 41 | **无优雅退出** | `close()` 只关 listener，`serve` 只在连接之间检查 `_closed`，在途请求无 drain 期限。与 #16 相关 |

### 客户端侧旧缺口

上文「三档：真缺失」整表仍然有效，未被本轮覆盖。其中与本轮相关的对应关系：

- `Url` path/query 与 `Url.Query()` → #23
- 请求侧 chunked（流式上传）→ #42
- `Response.content_length()` / `transfer_encoding()` / `location()` / `cookies()`、
  `Request.content_length()`、`Header` 多值访问器、`Request.Host` 覆盖、
  `Url.User` / `Url.Fragment`、`BasicAuth` / `AddCookie` / `FormValue` → 未建任务，按原表优先级排

## 明确不做

- **HTTP/2**：范围外，不考虑（`http.Pusher` 同）
- 上文「不打算做的」其余各项维持原判
