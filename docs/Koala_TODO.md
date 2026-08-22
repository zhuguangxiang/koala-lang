# Koala TODO：泛型运算符协议未完成项

> 记录泛型数值协议（Arithmetic / BitwiseOperators / Comparable）相关的未完成实现。
> 已打通部分见 `Koala_Design_Overview.md` 第 3 节：十六件二元运算符的 IR 下降链路
> （IR 协议指令 → KLR `num.*` → 字节码 `OP_NUM_*`）已实测完整。
>
> 更新日期：2026-08-22

---

## 1. VM 侧 `OP_NUM_*` handler 族（优先级最高）

**现状**：`vm_ops.h` 中仅 `OP_NUM_EQ` 有 TARGET handler，其余 15 条
（ADD/SUB/MUL/DIV/MOD、AND/OR/XOR/SHL/SHR、NE/LT/GT/LE/GE）缺失，
执行到时落入 `eval.c:176` 的 UNREACHABLE 分支直接 abort。

**影响**：
- `test_generic_9` 基线失败的直接原因；
- 任何经泛型 trait 约束的数值运算（如 `func f[T : Arithmetic](a T, b T) T { return a + b }`）
  编译正常、运行时崩溃。

**施工方向**：按 `OP_NUM_EQ` 的模式补齐 handler——经当前帧的 intf-table
O(1) 分发到具体类型的 `__add__` 等方法，保持零 call frame 语义。

---

## 2. 泛型一元运算符协议分发（`__neg__` / `__bitnot__`）

**现状**：`-a`（T : Arithmetic）报
`error: unary '-' operator requires int or float type.`；
`~a`（T : BitwiseOperators）报
`error: unary '~' operator requires int type.`
语义层直接拒绝泛型类型的一元运算。

**根因**：`OP_UNARY_*` 目前只是 IR 层伪指令，没有对应的协议下降路径与字节码族
（对标二元的 `OP_NUM_*`，需要一元版本）。

**施工方向**：语义层放开 trait 约束下的一元 `-` / `~`；新增一元协议指令
与对应 `OP_NUM_NEG` / `OP_NUM_BITNOT`（或归入同一族），isel 与 VM handler 同步。

---

## 3. 泛型复合赋值（`+=` / `-=` / `*=` / `/=` / `%=` 及位运算复合赋值）

**现状**：`var s = a; s += b`（T : Arithmetic）报
`error: inplace assignment is not supported for 'T' type.`

**根因**：复合赋值在泛型场景没有走 `__iadd__` 等协议方法的下降路径
（具体类型上的复合赋值工作正常）。

**施工方向**：泛型复合赋值下降为对应 `__i*__` 的协议调用
（或降级为 `s = s + b` 的协议二元运算 + 赋值），与第 1 项的 VM handler 族同步落地。

---

## 4. `in` 表达式的语义下降（`__contains__` 语法糖）

**现状**：`koala.y` 已有 `in_expr` 语法规则、AST 已有 `EXPR_IN_KIND`，
但 `parser_visit_expr` 与 irgen 的分发表都没有该 kind 的 handler——
写 `5 in r` 直接**段错误**（NULL handler 跳转地址 0）。

**影响**：运算符显式调用禁令（43 项，见设计文档第 3 节）落地后，
`__contains__` 不能显式调用，而 `in` 糖又未实现——
**dict/set 的成员判定目前没有任何可用入口**。

**施工方向**：为 `EXPR_IN_KIND` 补语义处理与 IR 下降，
下降为对 rhs 类型 `__contains__` 的方法调用即可。

---

## 依赖关系

```
第 2、3 项的运行时部分依赖第 1 项（VM handler 族）；
第 4 项独立，且因 __contains__ 显式调用被禁而成为功能性阻断，建议优先。
```

