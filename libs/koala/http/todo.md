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
| `http.Pusher` / HTTP/2 | 只做 HTTP/1.1 | 范围外 |
| `httptest.Server` | 测试服务器 | 需要 listener，独立话题 |
