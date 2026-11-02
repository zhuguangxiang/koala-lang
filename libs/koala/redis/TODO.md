# koala/redis TODO

Redis 客户端的设计、需求与进度记录。完成一项就把对应复选框打勾。

最后更新：2026-11-03

---

## 0. 当前进度总览

### 已完成

- [x] `__pkg__.toml`：包名 `koala/redis`，版本 `0.1.0`
- [x] `__redis__.kl`：包入口
- [x] `libs/koala/__ws__.toml`：packages 中加入 `"redis"`
- [x] `reply.kl`：`Reply` trait + 6 个具体回复类
- [x] `protocol.kl`：`encode` / `parse_reply`，`-` 返回 `ErrorReply`
- [x] `command.kl`：`Command` 值对象与单帧编码
- [x] `request.kl`：`Request`（Command 序列 / 管道）
- [x] `response.kl`：`Response`（Reply 序列）
- [x] `connection.kl`：字节级连接，唯一持有 socket 的地方
- [x] `client.kl`：重写为 `Request` / `Response` / `Command` 对象模型
- [x] `test/test_protocol.kl`：12 个单元用例
- [x] `test/test_command.kl`：7 个单元用例
- [x] `test/test_request.kl`：5 个单元用例
- [x] `test/test_response.kl`：3 个单元用例
- [x] `test/test_client.kl`：6 个真实连接用例（含 pipeline 与错误回复）
- [x] `wombat build redis` 通过
- [x] `wombat test redis` 通过：5 个文件、33 个用例全过
- [x] 真实服务端 `commandstats` 验证：PING/SET/GET/DEL/EXISTS/INCR/LPUSH/RPUSH/LLEN/LRANGE 均到达

### 待完成

- [ ] 验证无 Redis 时集成测试静默跳过（需停服，未实测）
- [ ] 后续版本 Backlog（见第 13 节）

---

## 1. v1 范围与目标

| 项 | 要求 |
|---|---|
| 协议 | 只实现 RESP2（`+ - : $ *`），兼容 Redis 2.0+ |
| 实现方式 | 纯 Koala，无原生 C 代码 |
| 通信模型 | 同步阻塞、单连接；一个 client 不跨协程共享 |
| 扩展预留 | 预留 RESP3 新类型、事务接入点，但 v1 不实现 |
| 入口 | `import "koala/redis"` |

**v1 明确不做**：RESP3、事务（MULTI/EXEC/DISCARD/WATCH/UNWATCH）、Pub/Sub、Cluster、Sentinel、TLS、Unix socket、UDP、自动重连。

---

## 2. 包结构

```
libs/koala/redis/
  __pkg__.toml          # [x]
  __redis__.kl          # [x]
  reply.kl              # [x]
  protocol.kl           # [x]
  command.kl            # [x]
  request.kl            # [x]
  response.kl           # [x]
  connection.kl         # [x]
  client.kl             # [x] 对象模型
  TODO.md               # [x]
  test/
    test_protocol.kl    # [x] 12 个用例
    test_command.kl     # [x] 7 个用例
    test_request.kl     # [x] 5 个用例
    test_response.kl    # [x] 3 个用例
    test_client.kl      # [x] 6 个用例，含 pipeline / error
```

---

## 3. 分层设计

### 3.1 总览

```
┌─────────────────────────────────────────────────────────────┐
│ client 层                       client.kl                   │
│ RedisClient：编排请求、类型化命令、错误策略                   │
│ 知道：Request / Response / Command / Reply                   │
│ 不知道：socket、帧格式                                       │
└──────────────────────────┬──────────────────────────────────┘
                           │ execute(Request) -> Response
                           │ exec(Command)    -> Reply
┌──────────────────────────▼──────────────────────────────────┐
│ redis 层                                                    │
│   command.kl   Command     命令值对象，负责单帧编码          │
│   request.kl   Request     Command 序列（管道）              │
│   response.kl  Response    Reply 序列                        │
│   connection.kl Connection 字节传输，唯一持有 socket 的地方  │
│ 知道：RESP 帧、Reply 类型、连接生命周期                      │
│ 不知道：具体命令语义、错误该不该抛、用户 API 形态            │
└──────────────────────────┬──────────────────────────────────┘
                           │ encode(args) -> bytes
                           │ parse_reply(BufReader) -> Reply
┌──────────────────────────▼──────────────────────────────────┐
│ 协议层                       protocol.kl + reply.kl         │
│   protocol.kl  RESP2 编解码，无任何 I/O 与连接概念            │
│   reply.kl     Reply trait + 具体回复类                      │
│ 知道：字节、CRLF、类型前缀                                   │
│ 不知道：socket、命令、请求序列、超时                         │
└──────────────────────────┬──────────────────────────────────┘
                           │
                     std/io（Buffer、BufReader、bytes）
                     std/socket 仅由 connection.kl 引入
```

依赖方向单向：**client → redis → 协议 → std**，禁止反向依赖。

### 3.2 边界清单

| 问题 | 归属层 |
|---|---|
| RESP 字节怎么拼、长度怎么读、CRLF 怎么校验 | 协议层 |
| `-ERR` 变成什么对象 | 协议层（`ErrorReply`） |
| "SET foo bar" 有哪些参数 | redis 层 `Command` |
| 多条命令怎么一次性发出、读几条回复 | redis 层 `Request` / client `execute` |
| socket 超时、关闭 | redis 层 `Connection`（client 透传） |
| `get` 返回 `str?` 还是 `str` | client 层 |
| 收到 `-ERR` 是返回还是抛 | client 层 |
| MULTI/EXEC 怎么组织 | 将来在 redis 层引入 `Transaction`，组合 `Request` |

### 3.3 层间禁令

- 协议层不得 import `std/socket`、不得知道 Command/Request/Client。
- `Command` / `Request` / `Response` 不得接触 socket。
- redis 层不得 import client 层。
- client 层不得直接操作 `BufReader` 或 socket，必须经 `Connection`。

---

## 4. 协议层需求（protocol.kl + reply.kl）

- [x] `encode(args list[str]) bytes`：编码为 RESP 数组 + 批量字符串
- [x] 长度使用**字节长度**（UTF-8 用例已验证）
- [x] `+` → `StatusReply`
- [x] `-` → `ErrorReply`（错误是数据，不 panic）
- [x] `:` → `IntReply`
- [x] `$` → `BulkReply`；`$-1` → `NilReply`
- [x] `*` → `ArrayReply`（递归）；`*-1` → `NilReply`
- [x] 批量字符串精确读满 payload + CRLF
- [x] 流中断 / 截断 / 未知类型字节 → panic
- [x] 测试：error reply
- [x] 测试：`*-1`
- [x] 测试：嵌套数组
- [x] 测试：多字节 UTF-8 字符串长度
- [x] 测试：空数组 `*0`
- [ ] 测试：截断/EOF 的 panic（测试框架目前不支持断言 panic）

扩展点：RESP3 的 map/set/double/bool/push 只需新增 `Reply` 子类并增加 `parse_reply` 分支，上层零改动。

---

## 5. 对象模型需求

### 5.1 Reply（reply.kl）— 已完成

- [x] `trait Reply {}`
- [x] `StatusReply{value str}`
- [x] `ErrorReply{value str}`
- [x] `BulkReply{value str}`
- [x] `IntReply{value int}`
- [x] `NilReply{}`
- [x] `ArrayReply{items list[Reply]}`（可嵌套）
- [x] 不使用 `any`，用 trait + 具体类表达异构树

### 5.2 Command（command.kl）— 已完成

- [x] 操作（op）+ 参数；纯值对象，不做 I/O
- [x] `__init__(name str)`
- [x] `arg(str) Command` / `arg_int(int) Command`（链式）
- [x] `op() str`
- [x] `args() list[str]`（副本）
- [x] `words() list[str]`（name 在前）
- [x] `to_bytes() bytes`（单命令一帧）
- [x] `static from_words(list[str]) Command`（空列表 panic）

### 5.3 Request（request.kl）— 已完成

- [x] 有序 `Command` 序列，代表"一次发送给 server 的 command 序列"
- [x] `add(Command) Request` / `add_words(list[str]) Request`
- [x] `len()` / `empty()` / `commands()`（副本）
- [x] `to_bytes()`：多帧拼接，一次 write
- [x] N 条命令 = N 条回复
- [x] 事务将来由 `Transaction` 组合 `Request`，不修改 `Request`

### 5.4 Response（response.kl）— 已完成

- [x] 有序 `Reply` 序列
- [x] `add(Reply)` / `len()` / `empty()`
- [x] `reply(index) Reply` / `first() Reply?`
- [x] `replies()`（副本）
- [x] `len()` 与对应 `Request` 一致

### 5.5 Connection（connection.kl）— 已完成

- [x] 持有 `so.Conn` + `io.BufReader`
- [x] `static open(host, port) Connection?`（失败返回 nil）
- [x] `send(frame bytes)`（写失败 panic，避免半帧）
- [x] `receive() Reply`（解析恰好一条）
- [x] `set_timeout(ms int)`（0 清除）
- [x] `close()`
- [x] 不感知具体 Redis 命令

### 5.6 RedisClient（client.kl）— 已完成

核心 API：

- [x] `static connect(host = "127.0.0.1", port = 6379) RedisClient?`
- [x] `execute(req Request) Response`
      - `_connection.send(req.to_bytes())`
      - 循环 `req.len()` 次 `receive()` 并 `Response.add`
      - 不因 `ErrorReply` panic，错误原样返回
- [x] `exec(cmd Command) Reply` = `execute(Request().add(cmd)).reply(0)`
- [x] `set_timeout(ms)` / `close()`

类型化便捷方法：

- [x] `ping() bool`
- [x] `echo(msg) str`
- [x] `get(key) str?`
- [x] `set(key, value) bool`
- [x] `del(key) int`
- [x] `exists(key) bool`
- [x] `incr(key) int`
- [x] `lpush(key, value) int`
- [x] `rpush(key, value) int`
- [x] `llen(key) int`
- [x] `lrange(key, start, stop) list[str]`

---

## 6. 错误语义（已确定）

- [x] 协议层：`-ERR` 解析为 `ErrorReply`，不 panic
- [x] 协议错误（EOF / 截断 / 未知类型）→ panic
- [x] `execute` / `exec` / `command`：`ErrorReply` 作为数据返回，不 panic
- [x] 类型化方法：遇到 `ErrorReply` panic，消息为 `"redis error: <server message>"`
- [x] 类型化方法：类型不符时给出明确 panic 信息

错误策略只存在于 client 层，不得渗透到协议层 / redis 层。

---

## 7. Pipeline 需求

- [x] `Request.to_bytes()` 支持多帧拼接
- [x] 一个 Request 多条 Command：单次 write，按序读 N 条回复
- [x] `Response.len() == Request.len()`
- [x] 管道中的错误逐条呈现，不影响后续回复读取

---

## 8. 连接生命周期

- [x] `connect` 失败返回 nil，不 panic
- [x] 默认无超时；`set_timeout(ms)` 同时作用于读写，0 清除
- [x] `close` 后不再发送
- [x] 不自动重连

---

## 9. 测试需求

### 9.1 单元测试（无需 Redis）

- [x] protocol：encode SET
- [x] protocol：status
- [x] protocol：integer
- [x] protocol：bulk
- [x] protocol：`$-1`
- [x] protocol：flat array
- [x] protocol：error
- [x] protocol：`*-1`
- [x] protocol：嵌套数组
- [x] protocol：UTF-8 字节长度
- [x] protocol：空数组 `*0`
- [x] command：链式构建 / `words()` / `to_bytes()` / `from_words` / 参数副本
- [x] request：`len/empty` / 单命令 / 多命令拼接 / 命令副本
- [x] response：`add/len/reply/first`（空时 nil）/ `replies()` 副本

### 9.2 集成测试（真实 Redis）

- [x] ping
- [x] set / get / del / exists
- [x] incr
- [x] list：rpush / lpush / llen / lrange
- [x] pipeline：SET/GET/DEL 一个 Request，校验 3 条回复及类型
- [x] 错误回复：对字符串 key 执行 LPUSH，校验 `ErrorReply`
- [ ] 无服务时静默跳过（代码已支持，未停服实测）
- [x] 不包含事务用例

### 9.3 验收命令

```sh
cd libs/koala && source ../../env.sh
wombat build redis
wombat test redis
```

真实到达验证：

```sh
redis-cli config resetstat
# 跑测试
redis-cli info commandstats
```

- [x] build 通过
- [x] test 通过（33 个用例）
- [x] commandstats 确认 PING/SET/GET/DEL/EXISTS/INCR/LPUSH/RPUSH/LLEN/LRANGE 真实执行

---

## 10. redis-cli 8.0.5 功能清单（参考）

redis-cli 是运维工具 + 命令执行器，不是库；没有连接池、自动重连、异步 API。

### 10.1 连接与传输

| 选项 | 功能 |
|---|---|
| `-h <hostname>` | 服务器地址，默认 127.0.0.1 |
| `-p <port>` | 端口，默认 6379 |
| `-s <socket>` | Unix domain socket，覆盖 `-h/-p` |
| `-t <timeout>` | 连接超时（秒，可小数），0 不限制 |
| `-u <uri>` | `redis://user:pass@host:port/db`；TLS 用 `rediss://` |
| `-4` / `-6` | DNS 解析优先 IPv4 / IPv6 |
| `-c` | Cluster 模式，跟随 `ASK` / `MOVED` 重定向 |

TLS：`--tls`、`--sni`、`--cacert`、`--cacertdir`、`--insecure`、`--cert`、`--key`、`--tls-ciphers`、`--tls-ciphersuites`。

### 10.2 认证、库与协议

| 选项 | 功能 |
|---|---|
| `-a` / `--pass` | 密码认证 |
| `--user <username>` | ACL 风格 `AUTH user pass`，需配合 `-a` |
| `--askpass` | 从 stdin 掩码输入密码 |
| `REDISCLI_AUTH` | 环境变量传密码 |
| `--no-auth-warning` | 关闭命令行带密码警告 |
| `-n <db>` | 选择数据库 |
| `-2` / `-3` | RESP2 / RESP3 |

### 10.3 输入方式

- 命令行参数直接执行
- 无参数 → 交互模式
- `-x`：最后一个参数从 stdin 读入（二进制安全）
- `-X <tag>`：指定 tag 参数从 stdin 读入
- `--quoted-input`：输入按带引号字符串解析
- 交互帮助：`help @<group>`、`help <command>`、`help <tab>`、`quit`
- 交互偏好：`:set hints` / `:set nohints`，配置在 `~/.redisclirc`
- 交互特殊命令：`clear`、`connect`、`disconnect`、`select`、`restart`、`exit`

### 10.4 输出格式

| 选项 | 功能 |
|---|---|
| 默认 | TTY 下格式化输出 |
| `--raw` | 原始输出；非 TTY 默认 |
| `--no-raw` | 强制格式化 |
| `-d <delimiter>` | raw 模式 bulk 分隔符，默认 `\n` |
| `-D <delimiter>` | raw 模式回复分隔符，默认 `\n` |
| `--csv` | CSV 输出 |
| `--json` | JSON 输出（默认 RESP3） |
| `--quoted-json` | JSON + ASCII 安全字符串 |
| `--show-pushes <yes/no>` | 是否打印 RESP3 PUSH 消息 |

### 10.5 执行控制

| 选项 | 功能 |
|---|---|
| `-r <N>` | 同一命令执行 N 次 |
| `-i <interval>` | 间隔（秒，可小数）；也用于 scan/stat 每轮、bigkeys 等每 100 轮 |
| `-e` | 命令失败时返回非零退出码 |
| `--verbose` | 详细模式 |
| `--version` / `--help` | 版本与帮助 |

### 10.6 管道与批量

| 选项 | 功能 |
|---|---|
| `--pipe` | 从 stdin 读取原始 RESP 数据批量灌入 |
| `--pipe-timeout <n>` | 等待回复超时，默认 30 秒，0 永久 |
| `-r` + `-i` | 重复批量执行 |
| `-x` / `-X` | shell 管道构造参数 |

### 10.7 脚本与调试

| 选项 | 功能 |
|---|---|
| `--eval <file>` | 执行 Lua，`keys , args` 用逗号分隔 |
| `--ldb` | Lua 异步调试器 |
| `--ldb-sync-mode` | 同步调试器（阻塞服务器、不自动回滚） |

### 10.8 监控与诊断

| 选项 | 功能 |
|---|---|
| `--stat` | 滚动打印内存、客户端等统计 |
| `--latency` | 持续采样延迟 |
| `--latency-history` | 延迟随时间变化 |
| `--latency-dist` | 延迟频谱图 |
| `--intrinsic-latency <sec>` | 系统固有延迟 |
| `--lru-test <keys>` | 模拟 80/20 缓存负载 |
| `--replica` | 从库视角显示主库命令流 |
| `--rdb <file>` | 拉取 RDB，`-` 表示 stdout |
| `--functions-rdb <file>` | 只拉 functions（不含 key） |

### 10.9 Key 扫描与容量分析

| 选项 | 功能 |
|---|---|
| `--scan` | 用 SCAN 列出所有 key |
| `--pattern <pat>` | 匹配模式，默认 `*` |
| `--quoted-pattern <pat>` | 可带引号、二进制安全 |
| `--count <n>` | 每次 SCAN 建议数量，默认 10 |
| `--bigkeys` | 找元素多的 key |
| `--memkeys` | 找内存大的 key |
| `--memkeys-samples <n>` | memkeys 采样元素数 |
| `--keystats` | 内存 + 长度统计 |
| `--keystats-samples <n>` | keystats 采样元素数 |
| `--hotkeys` | 找热 key，要求 LFU |
| `--cursor <n>` | 从指定 cursor 继续 |
| `--top <n>` | 输出前 N 大 key，默认 10 |

### 10.10 Cluster 管理（`--cluster <subcommand>`）

`create`、`check`、`info`、`fix`、`reshard`、`rebalance`、`add-node`、`del-node`、`call`、`set-timeout`、`import`、`backup`、`help`。

常用选项：`--cluster-yes`、`--cluster-replicas`、`--cluster-search-multiple-owners`、`--cluster-fix-with-unreachable-masters`、`--cluster-from/to/slots`、`--cluster-timeout`、`--cluster-pipeline`、`--cluster-replace`、`--cluster-weight`、`--cluster-use-empty-masters`、`--cluster-simulate`、`--cluster-threshold`、`--cluster-slave`、`--cluster-master-id`、`--cluster-only-masters`、`--cluster-only-replicas`、`--cluster-from-user/pass/askpass`、`--cluster-copy`。

---

## 11. 传输方式结论

### 11.1 UDP：redis-cli 不支持

- redis-cli 只有 TCP（`-h/-p`）、Unix socket（`-s`）、TLS over TCP（`--tls` / `rediss://`）。
- Redis 服务端不提供 RESP 的 UDP 监听；集群 bus、复制、TLS 均基于 TCP。
- 根因在 Redis 协议栈，不是 redis-cli 缺少选项。
- Koala 的 `std/socket` 本身支持 UDP（`SocketTypeSpec.udp()`、`listen_packet`），但 Redis 用不上。

### 11.2 TLS：redis-cli 支持，Koala 可接入但 v1 未做

redis-cli：

- 开关：`--tls`、`-u rediss://...`
- 证书：`--cacert`、`--cacertdir`、`--cert`、`--key`、`--insecure`
- 其他：`--sni`、`--tls-ciphers`、`--tls-ciphersuites`

Koala 现状：

- [x] `koala/tls` 包存在，基于 OpenSSL libssl
- [x] 对外 API：`probe() bool`、`dial(host, port) TlsConn?`（系统 CA + SNI）
- [x] HTTP 客户端 `https://` 已使用 `tls.dial`
- [ ] redis `Connection.open` 未接入 TLS
- [ ] `koala/tls` 缺自定义 CA / 客户端证书 / `--insecure` 选项

接入点：`Connection` 是 redis 包唯一接触 socket 的地方，只需在 `open` 中二选一（`so.dial` / `tls.dial`），上层无需改动。

限制：Redis 常用自签证书；没有自定义 CA / insecure 时真实环境大概率握手失败。若要纳入 v1，建议范围定为"系统 CA 的 TLS + secure 开关"。

### 11.3 Unix domain socket：redis-cli 支持，Koala 不支持

redis-cli：`-s <socket>`，覆盖 `-h/-p`。

Koala 现状（已核实）：

- [x] `std/socket` 不支持 AF_UNIX（已核实）
- `dial` 只接受 `host:port`，先 `split_host_port` 再 DNS 解析
- `_dial_one` / `listen` / `listen_packet` 只使用 `AF_INET` / `AF_INET6`
- `SocketAddr` 只有 `IpAddr + port`，无 Unix path 表示
- `SocketTypeSpec` 只有 `tcp/udp/raw/dgram`
- 原生 `src/modules/socket.c` 的 `_to_sockaddr` 只处理 `sockaddr_in` / `sockaddr_in6`
- 全仓库无 `AF_UNIX` / `sockaddr_un`

落地需要三处改动：

- [ ] 地址类型：新增 Unix path 地址（`UnixAddr` 或 `SocketAddr` 的 unix 变体）
- [ ] `dial` / `listen`：url 以 `/` 开头时走 `AF_UNIX + SOCK_STREAM`，跳过 DNS
- [ ] 原生 `socket.c`：`_to_sockaddr` 增加 `sockaddr_un` 分支

redis 侧接入点与 TLS 相同：`Connection.open` 增加一个分支。

---

## 12. redis-cli 能力与 Koala v1 对照

| redis-cli 能力 | Koala v1 |
|---|---|
| 基本命令执行、`-h/-p/-t` | 已有 `connect(host, port)` + `set_timeout` |
| 原始命令逃生通道 | `exec(Command(...))`；从词表可用 `Command.from_words(words)` |
| 管道（一次发送多条） | `Request` / `execute` |
| AUTH / SELECT / CLUSTER | 走 `exec(Command(...))`，无专用封装 |
| RESP2 | 支持 |
| RESP3（`-3`） | 不支持 |
| Unix socket（`-s`） | 不支持（std/socket 无 AF_UNIX） |
| TLS（`--tls`、`rediss://`） | 不支持（koala/tls 可接入，v1 未做） |
| UDP | 不支持（Redis 本身没有） |
| `-u` URI | 不支持 |
| 交互模式 | 不适用（库而非 CLI） |
| 输出格式化 / `--json` / `--csv` / `--raw` | 不适用 |
| `--scan` / `--bigkeys` / `--hotkeys` / `--stat` / `--latency` | 不做 |
| `--eval` / `--ldb` / `--pipe` / `--rdb` / `--replica` | 不做 |
| Cluster 管理 | 不做 |
| Pub/Sub | v1 不做（PUSH 依赖 RESP3） |

---

## 13. 后续版本 Backlog

### 事务（v1 不做，接口已预留）

- [ ] `Transaction` 对象，组合 `Request`
- [ ] MULTI / EXEC / DISCARD / WATCH / UNWATCH
- [ ] EXECABORT、WATCH 失效返回 nil、嵌套数组回复的语义处理

### RESP3

- [ ] map / set / double / boolean / push / attribute 回复类
- [ ] 协议协商（HELLO 3）

### 传输

- [ ] Redis over TLS（系统 CA + secure 开关）
- [ ] `koala/tls` 增加自定义 CA / 客户端证书 / insecure
- [ ] Unix domain socket（依赖 `std/socket` 先支持 AF_UNIX）
- [ ] 连接 URI（`redis://`、`rediss://`）

### 客户端能力

- [ ] AUTH / SELECT 专用辅助方法
- [ ] 重连 / 连接池
- [ ] Pub/Sub
- [ ] Cluster 重定向
- [ ] 更多命令的类型化封装（hash、set、zset 等）
- [ ] 扫描类辅助（SCAN 迭代器）

---

## 14. 验收标准

- [x] `wombat build redis` 通过
- [x] `wombat test redis` 全部通过
- [ ] 无 Redis 时集成测试静默跳过，套件仍通过（代码路径已实现，未停服实测）
- [x] 真实服务端 `commandstats` 能观察到测试发出的命令
- [x] client 层不直接接触 socket / BufReader
- [x] 协议层不 import `std/socket`
- [x] v1 无事务、无 TLS、无 Unix socket、无 UDP 代码
