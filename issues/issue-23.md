# 23. 全局可选变量判空收窄后使用，ir_cast panic（既有缺陷）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 与 [#25](issue-25.md) 同根因，建议合并修复

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

```
var gs str? = nil
func main() { gs = "hi"; if gs != nil { print(gs.len()) } }
```

编译崩溃：`panic: 'ir_cast' op requires a reg value or const`，**plain 与 --ssa 均触发**；与函数值无关，`str?` 即复现。

</details>

<details>
<summary><b>🧠 根因</b></summary>

`emit_ir_ident` 的 `SYM_SHADOW_VAR` LOAD 路径（`irgen.c` 约 580-588 行）对全局变量**直接取 `origin->ir_val`（`KLR_VALUE_GLOBAL`）送 `klr_build_cast`**，未先 `klr_build_get_global` 加载；cast 只接受 reg/const。

</details>

<details>
<summary><b>💥 影响</b></summary>

所有「**全局**可选变量判空后直接使用」的写法不可用（**局部变量正常**）。

与 #22 修复无关，修复前即存在。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

shadow LOAD 对 `VAR_SCOPE_GLOBAL` **先 `klr_build_get_global` 再按需 cast**（与 `SYM_VAR` LOAD 的全局路径一致）。

⚠️ STORE / LOAD_STORE 路径同理需检查。

</details>

<details>
<summary><b>🧪 建议测试</b></summary>

`test/test-run/test_global_optional_narrow.kl`，覆盖全局 `str?`/`int?`/函数? 判空后使用、else 分支收窄、循环内收窄。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 修复 #22 期间探测发现，**未修**，待讨论。

</details>