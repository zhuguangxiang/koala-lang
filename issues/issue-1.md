# 1. `[T]` 写列表类型，报错不友好

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 语法糖 `[T]` 未实现，表达式侧 `[str]()` 能否构造仍待定

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`func sub(path str, pairs [str]) {}` 报一串 `expected a valid function declaration.`、`expected key:value pairs.`、`expected ']'` 等，**没指出真正原因**。

</details>

<details>
<summary><b>✅ 正确写法</b></summary>

类型写 `list[str]`，构造写 `list[str]()`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

类型位置支持语法糖 `[T]`（等价 `list[T]`），**bison 验证无冲突**，风格同元组类型 `(A, B)`。

</details>

<details>
<summary><b>❓ 待定：表达式里的 `[str]()`</b></summary>

`[str]` 在表达式中是列表字面量（类型也是值，`[int]` 是合法的「类型列表」），**解析期无法区分**。

`[str]()` 只能在语义阶段特殊改写（callee 为单元素列表字面量、元素是类型、无参数），但会造成 `[str]()` 与 `let t = [str]` 含义不一致。

</details>