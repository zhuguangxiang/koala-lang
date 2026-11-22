# `koala/sqlite3` 遗留项

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 9 类缺口待做

> 来源：`docs/Koala_TODO.md` §85（2026-11-08，to do）

---

### 详细描述

<details>
<summary><b>📌 已记录的「有意差异」</b></summary>

`koala/sqlite3` 跟随 CPython 的 `sqlite3`（基于 vendored SQLite 3.54.1）。有意不同的地方包文档已写明：

- 错误是 nil（或错误码）加 `sqlite_errorcode()` / `sqlite_errorname()` / `errmsg()`，**不用异常**；
- 编程错误 panic；
- 一行永远是 `Row`；
- `named=` 携带具名参数的 dict；
- `in_transaction`、`total_changes`、`isolation_level` 都是**方法**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 尚未完成</b></summary>

- **游标迭代**：Python 的 `for row in cur.execute(...)` 没有对应物，`Cursor` 不是 `Iterable[Row]`。`while let row = cur.fetchone()` 可用。
- **模块属性**：`apilevel`、`paramstyle`、`threadsafety`、`complete_statement()`。
- **类型适配**：`detect_types`、`PARSE_DECLTYPES` / `PARSE_COLNAMES`、`register_adapter`、`register_converter`、`text_factory`，以及 `Row` 之外的 `row_factory`。
- **回调进 Koala**：`create_function`、`create_aggregate`、`create_window_function`、`create_collation`、`set_authorizer`、`set_progress_handler`、`set_trace_callback`。**native 代码目前无法回调进 Koala**（见 `Koala_TODO.md` §80 的 `koala/http` parser）。
- **其它 `Connection` 方法**：`interrupt`、`backup`、`iterdump`、`blobopen`、`getlimit` / `setlimit`、`serialize` / `deserialize`，以及 Python 3.12 的 `autocommit` 属性。
- **`Row` 的相等与哈希**：Python 按列与值比较两行。
- **上下文管理器**：Python 的 `with conn:` 提交或回滚。**等 `with` 落地**。
- **阻塞**：每次调用都阻塞线程；一旦 Koala 有并发 API，长查询会拖住调度器。
- **`fetchmany` / `fetchall` 出错时**返回 nil，出错前读到的行会丢失。Python 抛异常也一样丢；需决定是否保留。

</details>