# 退出码模型（待定）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— panic 后仍退出 0；`main` 的返回值被丢弃

> 来源：`docs/Koala_TODO.md` §19（待定）

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`sys.exit(code = 0)` 已实现（`libs/std/sys/__sys__.kl` + `src/modules/sys.c`），但退出码通道**不完整**：

- `kl_run_main`（`src/runtime/vm.c`）在异常路径只打印异常，`koala_run_file` 恒返回 0 —— panic 的 Koala 程序在 shell 看来是**成功退出**。
- `main` 的返回值被丢弃（`kl_run_main` 拿到 `TValue` 只判 `is_error`），`main` 签名不受约束。

</details>

<details>
<summary><b>📅 Update (2026-11-06)</b></summary>

`koala x.kl` 在 panic 之后仍然 exit 0；而 `koala x.kl ++ --test` 在某个 `@test` 失败时 exit 1。

`test-run/test_shift_traceback.kl` 在顶层 panic，因此它的 RUN 行没有 `!` —— 第 1 项做完后需要补上。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待定项</b></summary>

1. **panic 退出码改为非 0**（如 1）：`kl_run_main` 异常路径返回 1，`koala_run_file` 透传。否则工具链分不清「正常退出」与「崩溃」，`sys.exit` 的价值被削弱。
2. **是否支持 `func main() int` 作为退出码通道**：C/Go 血统，与「可预期失败用返回值」一致；代价是 parser 需加 `main` 签名约束。保底方案：明确规定 `main()` 无返回，退出码只有 `sys.exit` 一条通道。

</details>

<details>
<summary><b>🧪 测试</b></summary>

退出码无法用现有 `--test` 断言，需 lit 层 `sh` 跑独立进程比对 `$?`。

</details>