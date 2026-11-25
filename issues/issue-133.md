# 133. native C 测试脱离回归：wombat 只收 `test/*.kl`，re 的 180 个 C 用例无人调用

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复：`koala/re/native/test/build-and-test.sh`（180 用例）不被任何聚合脚本调用
- [ ] 未修复：wombat 无 native 测试钩子，缺口是机制性的，新增 native 测试会默认漏掉
- [ ] 未治理：编译产物 `test_re_core`（258KB）直接落在源码目录 `native/test/`
- [ ] 待补测试：接线验证（改坏 C 源码后聚合脚本必须失败）

> 来源：2026-11-24 解耦 koala-libs/apps 对 koala-lang 源码树的依赖时，重写 re 的 C 测试脚本后查引用发现。

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`koala/re` 的 C 层单测 `native/test/test_re_core.c`（**180 个用例**）不被任何聚合脚本调用，只能手动跑：

```bash
cd koala-libs/koala/re/native/test && ./build-and-test.sh
# 180 tests, 180 passed, 0 failed
```

而 `koala-libs/koala/build-and-test.sh` 跑的是 `wombat build` + `wombat test`（JIT 开/关两轮，91 个 **koala 层**用例），其中 re 包只收 `re/test/` 下 5 个 `.kl`：`test_re` / `test_find` / `test_split` / `test_sub` / `test_unicode`。

于是「91/91 全绿」这个信号里**完全不包含那 180 个 C 用例**，`build-std-koala.sh` 同样跑不到。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

```bash
cd /home/james/Codes/koala-libs/koala
./build-and-test.sh 2>&1 | grep -c test_re_core
# 0 —— 聚合回归的输出里没有它的任何痕迹

grep -rn 'native/test\|test_re_core' /home/james/Codes/koala-libs \
    --include='*.sh' --include='*.toml' --include='*.kl' \
    --exclude-dir=build --exclude-dir=dist
# 全部命中都在 native/test/build-and-test.sh 自身内部，无外部调用者

ls -d /home/james/Codes/koala-libs/koala/*/native/test
# 只有 re —— 目前唯一一个 native C 测试
```

2026-11-24 实测（日志 `/tmp/klibs_orphan_check.log`）：聚合脚本 `EXIT=0`，两轮各 `91 passed, 0 failed, 91 total` / `27 package(s) tested`，全日志中 `test_re_core` 命中 **0** 次、`180 tests` 命中 **0** 次。同一时刻手动跑 C 测试为 `180 tests, 180 passed, 0 failed`——两者互不相干。

</details>

<details>
<summary><b>🧠 根因</b></summary>

1. `libs/std/wombat/test.kl:87`：`if !name.ends_with(".kl") { continue }` —— 测试发现**只认 `.kl` 后缀**，C 测试天然被过滤。
2. `libs/std/wombat/test.kl:53`：扫描目录是 `<ws_root>/test/`（包级同理 `<pkg>/test/`），**`native/test/` 根本不在扫描范围**。
3. `__pkg__.toml` 没有声明 native 测试的字段，wombat 也没有对应钩子。
4. `koala-libs/koala/build-and-test.sh` 只调 wombat，**没有兜底遍历** `*/native/test/build-and-test.sh`。

四层都不覆盖，所以 C 测试成了孤儿。

</details>

<details>
<summary><b>💥 影响</b></summary>

- 现状是 1 个包 / 180 个用例脱离回归，但**缺口是机制性的**：将来任何新增 native C 测试都会默认漏掉，且没有任何提示。
- `re_core.c` / `re_unicode.c` 是正则引擎核心，C 层 180 用例覆盖回溯、Unicode 边界等分支，koala 层 5 个 `.kl` 覆盖不到。**实际回归保护弱于「91/91 全绿」的表面印象**。
- 附带：编译产物 `test_re_core`（258KB 二进制）直接生成在源码目录 `native/test/`，无清理约定，源码与产物混杂。

</details>

---

### 修复方案

<details>
<summary><b>🛠 候选方案</b></summary>

按侵入性从低到高：

1. **聚合脚本兜底**（推荐先做，最小改动）：`koala-libs/koala/build-and-test.sh` 在 `wombat test` 之后遍历 `*/native/test/build-and-test.sh` 并执行，任一非 0 即整体失败。不动 wombat，对将来新增的 native 测试自动生效。缺点是 C 用例数与 koala 用例数分开统计。
2. **wombat 原生支持**（更彻底）：约定扫描 `native/test/build-and-test.sh`，或在 `__pkg__.toml` 增加 `native_tests = [...]` 字段，由 `wombat test` 统一调度，使 C 用例进入同一份统计。需先决定失败计数如何与 `.kl` 用例合并、以及是否也跑 JIT 关闭那一轮（C 测试与 JIT 无关，应只跑一次）。
3. **产物治理**：脚本把可执行文件输出到 `native/test/build/`，或运行结束即删除，避免二进制留在源码目录。

建议先做 1 + 3 止血，2 作为后续统一。

</details>

<details>
<summary><b>🧪 建议新增测试</b></summary>

- **接线验证（关键）**：临时让 `re_core.c` 的匹配入口恒返回失败，`koala/build-and-test.sh` 必须非 0 退出；恢复后全绿。这是确认「C 测试真的进了回归」的唯一可靠手段——只看输出里有没有 `test_re_core` 字样不足以证明失败会传播。
- wombat 侧（若做方案 2）：在 `libs/std/wombat/test/` 加用例，构造一个含 `native/test/build-and-test.sh` 的假包，断言它被调用，且脚本返回非 0 时 `wombat test` 整体失败。
- 产物：断言脚本运行后源码目录无新增二进制。

</details>

<details>
<summary><b>🩹 绕过办法</b></summary>

手动跑 `cd koala-libs/koala/re/native/test && ./build-and-test.sh`。改动 `re_core.c` / `re_unicode.c` 后**必须记得手动执行**，聚合回归不会提醒你。

</details>