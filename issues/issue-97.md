# `koala/http` / `koala/tls` / `koala/sqlite3` 的 native handle 没有 finalizer

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 等运行期有 free 通道后再补 `_free`

> 来源：`docs/Koala_TODO.md` §79（2026-11-08，waits for a free path）

---

### 详细描述

<details>
<summary><b>🌐 `koala/http`</b></summary>

`_Parser`（`parser.kl`）包着一个由 `http_parser_new` 用 `calloc` 分配的 `HttpParser`。只有 `close()` 会释放它。

`Response.parse`、`ResponseHead.parse` 与客户端在**每条路径**上都会 close；但一个被丢弃且未 `close()` 的 `_Parser` 会泄漏。

</details>

<details>
<summary><b>🔒 `koala/tls`</b></summary>

`TlsConn` 持有一个 `SSL *`。只有 `close()` 会释放（`SSL_shutdown` + `SSL_free`）并关闭 TCP fd；被丢弃且未 `close()` 的 `TlsConn` **两者都泄漏**。

</details>

<details>
<summary><b>🗄 `koala/sqlite3`</b></summary>

`Connection` 持有一个 `sqlite3 *` 及其缓存的语句，`Cursor` 持有它正在跑的语句。只有 `Connection.close()` 会释放：关闭其游标、finalize 缓存、调 `sqlite3_close_v2`。

被丢弃且未 `close()` 的连接会泄漏数据库 handle、其语句，若是文件还会**保持文件打开**；未提交的事务会一直悬着直到进程退出。Python 在垃圾回收时关闭。

</details>

---

### 修复方案

<details>
<summary><b>🛠 方向</b></summary>

与 §28（`os.run` 子进程管道的 fd 泄漏与 zombie，尚未迁移）**同类泄漏**：等运行期有了 free 通道，给三者都加上 `_free`。

</details>