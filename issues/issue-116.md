# 未初始化变量的读取：无诊断 + 垃圾值 / nil / 两处段错误（待修）

← 返回 [问题列表](issues.summary.md)

- [ ] 待修 —— 4 个独立问题，其中 2 个是段错误；规则已拟定待作者确认

> 来源：`docs/Koala_TODO.md` §93（2026-11-12，待修）
>
> **编译器对「声明了但没赋初值就读取」完全不报错。** 全仓库 grep 不到 `uninitialized` / `not initialized` / `used before` 之类诊断，`VarSymbol`（`include/parser/symbol.h:82`）里也没有任何「已赋值」状态位。

---

### 详细描述

<details>
<summary><b>📊 现状矩阵（93.1）</b></summary>

| 写法 | koalac | VM | SSA |
| --- | --- | --- | --- |
| 局部 `var a int` 后 `print(a)` | exit 0，无诊断 | 打印 **40960**（垃圾） | **段错误** |
| 局部 `var s str` 后 `print(s)` | exit 0，无诊断 | 打印 **40960**（垃圾当指针） | 未测 |
| 全局 `var g int` 后 `print(g)` | exit 0，无诊断 | 打印 **`nil`** | 打印 **`nil`** |
| 全局 `var g int` 后 `print(g + 1)` | exit 0，无诊断 | **运行时断言崩溃** | **运行时断言崩溃** |
| 全局 `let c int`（无 `=`） | 已报错 `expected '='` | — | — |
| 局部 `let c int`（无 `=`） | **段错误** | **段错误** | **段错误** |

全局算术崩溃的断言：

```
include/runtime/vm_ops.h:85: _eval_frame: Assertion
  `((((regs + rs)->tag) & ~(uintptr_t)0b11) == 0b1000) || ... == 0b1100)' failed.
```

即一个声明为 `int` 的全局槽里装着 `nil`，是**类型谎言**，不只是「值为 0」的问题。

---

### 修复方案

<details>
<summary><b>❌ 缺陷 A：局部 var 无初值被读取，无诊断（待修）</b></summary>

- [ ] 最小复现：

  ```kl
  func main() {
      var a int
      print(a)      // VM: 40960    SSA: 段错误
  }
  ```

- [ ] **前端根因**：`src/parser/parser.c:1392` `parse_var_decl()`，`!exp && ts` 分支只设 `sym->ts` 就 `sym->status = SYM_RESOLVED` 返回，不记录「有没有初值」。
- [ ] **后端根因**：`src/parser/irgen.c:2711` `emit_ir_var_decl()`，局部变量只 `klr_build_local_var()` 建槽，`exp` 为空就不发 `move`，槽里是未初始化内存。
- [ ] **SSA 崩溃栈**（`koalac --ssa`，gdb）：

  ```
  find_entry            src/common/hashmap.c:163
  hashmap_get           src/common/hashmap.c:177
  read_variable         src/parser/klr/ssa.c:85
  read_variable_recursive src/parser/klr/ssa.c:243
  read_variable         src/parser/klr/ssa.c:93
  build_ssa             src/parser/klr/ssa.c:318
  ```

  `read_variable_recursive()` 一路回溯到入口仍找不到该变量的任何定义。注意 `build_ssa()` Step 2（`ssa.c:273`）已给**所有**块初始化过 `current_defs`，所以不是「map 没初始化」那么简单，回溯 path 上的 `pred` 有问题，具体机制未查清。前端加上诊断后合法程序不会再走到这里，但 SSA 侧本身是否该有兜底另议。

</details>

<details>
<summary><b>❌ 缺陷 B：全局 var 无初值，槽里是 nil（待修）</b></summary>

- [ ] 最小复现：

  ```kl
  var g int
  func main() { print(g) }      // nil
  ```

  ```kl
  var g int
  func main() { print(g + 1) }  // 运行时断言崩溃，见现状矩阵
  ```

- [ ] 全局 `let` / `const` 无初值**已经**被文法拦住（`src/parser/koala.y:1524` 起的 `let_decl` 所有非错误产生式都要求 `'='`），只有 `var` 开着这个口子。

</details>

<details>
<summary><b>❌ 缺陷 C：局部 `let` 无初值 → 编译器段错误（待修，独立小 bug）</b></summary>

- [ ] 最小复现：

  ```kl
  func main() {
      let c int
      print(c)
  }
  ```

  `koalac` 直接 SIGSEGV，**连那条 `expected '='` 都没打出来**。

- [ ] 根因：`src/parser/koala.y:1554` 的错误产生式 `LET ID optional_type error` 报完 `expected '='` 后置 `$$ = NULL`；`koala.y:2757` 的 `local: let_decl semi` 动作里 `var_set_where($$, VAR_LOCAL)` 是个裸宏（`include/parser/ast.h:537`：`((VarDeclStmt *)stmt)->where = _where;`），**不判 NULL**。
- [ ] 旁证：紧邻的 `local: const_decl semi`（`koala.y:2752`）里同一行**已经被注释掉了** —— 这个坑之前有人踩过，但只绕开了 const，没修 let / var。
- [ ] `var_decl` 的错误产生式（`koala.y:1741` 起）同样置 `$$ = NULL`，`koala.y:2762` 同样无保护。实测 `var a int =` 走到别的产生式没崩，但这条路径的 NULL 风险仍在，宜一并加保护。

</details>

<details>
<summary><b>✅ 必须保留的合法写法（实测 VM / SSA 都正确，93.5）</b></summary>

- [ ] 局部「先声明后赋值」是合法且常用的，修复**不能**简单地「没初始化式就报错」：

  ```kl
  var a int; a = 5; print(a)                    // 5
  var a int; if true { a = 1 } else { a = 2 }   // 1
  var a int; for i in range(3) { a = i }        // 2
  ```

- [ ] 全局可选类型的懒初始化，**全仓库唯一的无初值全局**，在 std 里：`libs/koala/log/default.kl:75` 的 `var _default Logger?`，靠 `if _default == nil { _default = Logger(...) }`（`:80`）懒建。对 `T?` 来说未初始化就是 nil，语义自洽，**不能打断**。

</details>

<details>
<summary><b>📋 拟定规则（未施工，待作者确认，93.6）</b></summary>

- [ ] 1. **全局非可选**必须有初始化式，否则报错 —— 堵住缺陷 B 的类型谎言。
- [ ] 2. **全局可选 `T?`** 允许省略初值，等价于隐式 `= nil`，std 不动。
- [ ] 3. **局部 `var`**：给 `VarSymbol` 加「已赋值」标记，赋值语句置位；**整个函数里从未被赋值过**的变量被读取时报错。
  这是小步方案，漏判 `if c { a = 1 } print(a)` 这类部分路径未赋值；完整的必经赋值分析（definite assignment）要新增流分析 pass，且会**拒掉**「循环内赋值、循环外读取」这种现在能跑的写法，暂不做。
- [ ] 4. 顺手修缺陷 C 的 NULL 解引用（`var_set_where` 加保护，或错误产生式不返回 NULL）。

</details>

<details>
<summary><b>💰 施工代价与影响面（93.7）</b></summary>

- `test/test-kl/test_call.kl:59,61` 有两个 `var l2 List[Bar];` / `var l2_1 List[Baz];`，**声明后从未使用**（遗留装饰）。规则 1 会让它们各多出一条错误，打乱该负向用例钉住的报错序列，需要补初始化式或删掉这两行。
- 除此之外 `libs/` / `test/` / `demo/` 里没有别的无初值全局。
- 按 TDD：先写用例（局部未赋值读取、全局非可选无初值、局部 let 无初值不崩、三种合法写法 + std 懒初始化仍通过），再实现。

</details>

<details>
<summary><b>🔗 关联条目（93.8）</b></summary>

- [#94](issue-94.md)（§92）也是「读到一个不该读的值」，但那个是 SSA 优化把两个变量合并所致，与本节无关：本节在**前端就没有任何检查**，两条后端都受影响。
- 参数 / 局部变量的其它诊断缺口：§75（unary 仍自动提升，已修）。

</details>
</details>