# 10. `ut.assert_eq` 传入可选类型时 koalac 断言崩溃

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 仍断言崩溃，已用 `next_strict()` 绕过

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`ut.assert_eq(dq.iter().next(), "v")`（`next()` 返回 `str?`）时 koalac 触发：

```
check_generic_bound: Assertion 'ts_sym && ts_sym->ts && ts_sym->ts->kind == TYPE_TYPE' failed
```

位置 `parser_expr.c:1847`。**应报类型错误，而不是断言崩溃。**

</details>

<details>
<summary><b>🩹 绕过办法</b></summary>

改用 `next_strict()`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

（原记录未给出方案，待补充。目标是把断言失败换成可读的类型错误。）

</details>