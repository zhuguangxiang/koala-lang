# `--inline` 支持多个基本块的函数（暂缓）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现（暂缓）—— `can_inline` 要求 `func_num_blocks(callee) == 1`

> 来源：`docs/Koala_TODO.md` §96（2026-11-13，暂缓）

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`src/parser/opt/inline.c` 的 `can_inline` 要求 `func_num_blocks(callee) == 1`：被调函数里只要有 `if`、`&&`/`||`、`?:`、循环、`!` 等分支，就不内联。

get/set 已覆盖，但带判断的小函数（如 `if x < 0 { return 0 }; return x`）还不能内联。

</details>

---

### 修复方案

<details>
<summary><b>🛠 做法</b></summary>

- [ ] 在调用点把调用方的 bb 切成两半（前半块 / 后半块）。
- [ ] 克隆被调函数的全部 bb，参数替换为实参，块之间的边一并克隆。
- [ ] 每个 `ret` 改为跳到后半块；有返回值时在后半块开头用 phi（或一个 local）汇合返回值。
- [ ] 规模限制：按总指令数和 bb 数设上限；递归、尾调用不内联。
- [ ] 测试：if / 短路 / `?:` / 循环 / 多个 return 的函数和方法，`--dump=inline-ir` 加运行结果。

</details>

<details>
<summary><b>⚠️ 风险</b></summary>

- 引入新的控制流与 phi，可能再暴露 opt / regalloc 的问题（参见本轮修过的死块删除、`new` 寄存器分配）。
- 依赖 §95：多 bb 函数出错时丢帧更难定位。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#91](issue-91.md)（§97 泛型）、[#92](issue-92.md)（§99 闭包 call）：解析出 callee 后仍受本条限制。

</details>