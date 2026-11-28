# Koala Tensor 待办（V1-min）

> 临时集中记录，按要求先落盘、后面统一处理。按 `.clinerules` 4.1，正式问题入口是 `issues/`
> （编号从 `issues/issues.summary.md` 页首取），统一处理时把下面各条迁进去。
>
> 当前进度：Tensor 已迁出 builtin，成为独立包 `koala/torch`（见下 §0）。
> 核心全量回归 646/646 通过，`koala-libs/koala/torch/test/test_tensor.kl` 13/13 通过。

---

## 0. 本轮已完成：Tensor 迁出 builtin，成为独立包 `koala/torch`

- [x] 全部搬走：`tensorobj.c` / `storage.c` / `device_cpu.c` + 三个头文件 + `tensor.kl`
- [x] 编译器里 Tensor 的特殊地位全部删除（不再是关键字）
- [x] 核心全量回归 646/646；torch 包测试 13/13

**新位置**（`~/Codes/koala-lab/koala-libs/koala/torch/`）：

| 文件 | 来源 |
| --- | --- |
| `native/tensorobj.c` | `src/objects/tensorobj.c` |
| `native/storage.c` | `src/runtime/storage.c`（删掉唯一的 `log_debug`，见下） |
| `native/device_cpu.c` | `src/runtime/device_cpu.c` |
| `native/{tensorobj,storage,device}.h` | `include/runtime/` 同名头文件 |
| `native/torch.c` | 新增：`torch_native_lib_init` → `kl_reg_type(lib, &tensor_type)` |
| `tensor.kl` | `libs/std/builtin/tensor.kl` |
| `__torch__.kl` | 新增：包文档 + `link "koala_torch"` |
| `__pkg__.toml` | 新增：`[native] name = "koala_torch"`，照 `koala/zlib` 的先例 |
| `test/test_tensor.kl` | `test/test-ut/test_tensor.kl`（加 `from "koala/torch" import Tensor`） |

**核心侧删除**：

- `koala.l`：`"Tensor"` 词条、自动分号 token 表里的 `TENSOR`
- `koala.y`：`%token TENSOR`、`%type tensor_type`、`tensor_type` 四条产生式、
  `atom_type` 里的 `| tensor_type`、`CLASS TENSOR '[' ID ']'` 专用产生式、`atom` 里的 `| TENSOR`
- `parser.c`：`class 'Tensor' can only be declared in std/builtin` 守卫
- `src/CMakeLists.txt` 三行、`src/modules/builtin.c` 的 include 与 `&tensor_type`
- `test/test-kl/test_tensor_type_syntax.kl`、`test/test-kl/test_tensor_class_reserved.kl`（测的就是被删的特性）

**用法变化**：`Tensor` 现在是普通标识符，必须 `from "koala/torch" import Tensor`
（或 `import "koala/torch"` 后写 `torch.Tensor`）。不写 import 时 `let x Tensor = 1` 报
`'Tensor' is not found`；用户包现在可以自己声明 `class Tensor[T]`（已实测）。
裸 `Tensor` 不再等于 `Tensor[float32]`，见 §13。

**构建/安装流程**（已实测）：`./build-debug.sh` → `./install-debug.sh` →
`cd libs/std && wombat build --force` → `cd koala-libs/koala/torch && wombat build` →
`koala-libs/install.sh`。

**本轮新发现（非 tensor，建议进 `issues/`）**：

- wombat 不检测**源文件删除**：删掉 `libs/std/builtin/tensor.kl` 后 `wombat build` 仍报
  `12 up to date`，产物里还留着旧 Tensor 类，必须 `--force`。`up_to_date` 只比现有输入的 mtime。
- 外部 native 包**不能用 `log.h`**：`libkoala_common` 是静态库链进 `libkoala.so`，`_log_log`
  不在动态符号表，dlopen 后报 `undefined symbol: _log_log`。`storage.c` 里唯一一处 `log_debug` 已删。
  要么导出日志符号，要么在文档里写明外部 native 禁用 `log.h`。

---

## P0 — 阻塞或有正确性风险

### 1. storage 压力不会触发 GC，短命张量循环必炸

- [ ] 待定方案并修复

**现象**：storage 字节数在 GC 预算之外，分配 storage 不触发 GC。循环里不断创建短命张量时，
GC 只看到几十字节的张量头，迟迟不回收，storage 计数涨到上限后 `storage_new` panic。

**最小复现**（`KOALA_STORAGE_MAX=1M`）：

```kl
for i in range(2000) {
    let t = Tensor.full[float32](1.0, 10000)
    if t.at(9999) != 1.0 { print("bad") }
}
// panic: storage: out of memory on cpu, used 1040000 + 40000 bytes exceeds max 1048576 bytes
```

循环内加 `import "std/gc"` + `gc.collect()` 就能跑完 → `_free` 钩子本身是对的，缺的是触发。

**候选方案**：

- A：storage 字节折算进 GC 分配计数/阈值，走常规分配路径触发回收。改动最小，但把 storage 语义掺进 GC。
- B：`storage_new` 在用量超过软水位（如 max 的 1/2）时主动 `gc_collect` 一次，再判预算。分层干净，storage 单向依赖 GC。**（倾向）**
- C：维持现状，靠 `free()` / 显式 `gc.collect()`，文档写明。V1-min 可接受，但训练循环必炸。

**建议测试**：`koala-libs/koala/torch/test/test_tensor_gc.kl`，`KOALA_STORAGE_MAX=1M` 下跑上面的循环（不显式 collect），`CHECK: done`。

### 2. `free()` 与视图共享的所有权规则未定

- [ ] 定规则（阻塞 reshape/view/transpose/narrow/select/expand）

现在每个张量独占 storage，`free()` 直接释放整块。views 落地后，释放一个视图会悬空基张量。
需要先定：共享标记放哪（storage 引用计数 / 张量 flags）、`free()` 语义（只减引用还是强制释放）、
以及 `tensorobj.h` 里没有 refcount 字段该怎么补。`tensorobj.c` 里已留注释标注该处。

### 3. 元素类型参数 `T` 被 native 忽略

- [ ] 非 float32 应报错而不是静默降级

`Tensor.zeros[int64](2)` 现在静默返回 float32 张量（native 恒建 `TENSOR_F32`，不看 `U`）。
V1-min 只支持 float32，应当显式报错「only float32 is supported」，否则用户拿到错类型的结果。
需要确认静态 native 方法能否拿到实例化后的类型实参。

---

## P1 — 功能缺口

### 4. `fmt` 输出格式（`to_str` 已删除）

- [x] 删除 `to_str`：`to_str` 是 convert 语义，Tensor 没有「转成字符串」的能力，文本形式只有 `fmt`
- [x] 元素按张量自己的 dtype 打印，不 widen（已修）
- [ ] 是否截断大张量、是否显示 dtype
- [ ] `.0` 是否全局加（语言级，见下）

现输出 `Tensor[2, 3]([[1, 2], [3, 4]])`，0 维是 `Tensor[](3.5)`，free 后是 `Tensor[freed]`。文档没规定。

**`to_str` 已删**：`tensor.kl` 的声明和 `tensorobj.c` 的 `_tensor_to_str` 都已移除，std dist 已重编。
取文本只能 `print(t)` / `format("{}", t)`；`t.to_str()` 现在编译期报
`'to_str' is not found in instance of class 'Tensor[float32]'`。测试改用 `format("{}", t)`。

**已修**：`tensor_fmt_dim` 原先把 float32 元素 widen 成 `float64_value` 再 fmt，走 `%.16g`，
`Tensor.full[float32](0.1, 3)` 打印成 `0.1000000014901161`。现按 `t->dtype` 用 `float32_value`（`%.7g`），
输出 `Tensor[3]([0.1, 0.1, 0.1])`；回归断言见 `test_tensor_fmt_float32`。
`at()` / `item()` / `tolist()` 返回 `float64` 是 `tensor.kl` 签名定的，widen 噪声与 numpy 一致，不改。

**`.0` 待定（语言级，不是 tensor 的事）**：`_float_to_buf`（`src/objects/floatobj.c:16`）用 `%g`，
会吃掉小数点，所以 `print(0.0)` → `0`、`print([1.0, 2.0])` → `[1, 2]`，tensor 只是复用同一行为。
加 `.0` 的好处是能区分 int/float、序列化回读不丢类型（Python/numpy/torch 都带）；
代价是 `_float_to_buf` 同时供 fmt/to_str/str()，粗查 37 个测试文件打印 float 字面量，
CHECK 行和 `docs/std_builtin_api.md:3584` 的 `to_str` 契约都要跟着改。
**结论：tensor 不特殊化**（否则 `print(t)` 带 `.0`、`print(t.tolist())` 不带，同一张量两种样子）；
要加就全局加，作为独立决定。

### 5. 大量 `@native` 方法仍是 not_impl 陷阱

- [ ] 按 M2–M5 逐步实现

`tensor.kl` 已声明但未实现：views（reshape/view/transpose/unsqueeze/squeeze/narrow/select/expand）、
copies（contiguous/clone/copy_）、elementwise（`__add__` 等 / neg/exp/log/sqrt/abs/relu/sigmoid/tanh）、
reductions（sum/mean/max/min/sum_dim/mean_dim）、matmul、in-place（`__iadd__` 等 / fill_/zero_）、allclose。
调用报 `func 'Tensor::xxx' is not implemented!`。

### 6. `storage_used()` 未暴露给 koala

- [ ] 定放哪个包（迁包后 `std/gc` 不能反向依赖 `koala/torch`）

现在只能靠 panic 信息看用量，没法写「分配后用量增加、free 后回落」的断言。
storage 已在 `koala/torch`，`storage_used()` 放 `torch` 最顺；放 `std/gc` 会让标准库依赖外部包。

### 7. C 层测试未落地

- [ ] `/tmp/st_test.c` → storage 的 C 测试（位置待定，见下）
- [ ] 新增 tensor 的 C 测试（内联/ext dims 边界、numel 溢出、free 后访问）
- [ ] 注册进构建

迁包后 `koala-libs/koala/torch` 没有 `test/test-c` 那套 ctest 基础设施，
要么在 torch 包里搭一套 native 测试，要么把 storage/tensor 的 C 测试留在 koala-lang 并链 torch 的源文件。

---

## P2 — 文档、环境与遗留

### 8. `docs/koala_tensor.md` 与代码不一致（按要求本轮未改文档）

- [ ] 统一处理时同步

- 文档的 `SHARED` 标志、flags/version/ready_event/deleter 字段，代码里没有。
- **文档仍写 Tensor 是编译器内置关键字、壳在 `std/builtin/tensor.kl`**（§4.1、D1、V1 落地清单），
  实际已迁到 `koala/torch` 且关键字已删，`docs/koala_ml.md` §4.1 同改。
- `docs/std_builtin_api.md` 里的 Tensor 条目要移到 koala/torch 的 API 文档。
- 文档的 `StorageObject` / `DeviceBackend` 抽象与现有 `storage.h` / `device.h` 不一致。
- 文档 gc.h API 表与实际 `gc.h` 不符。
- `docs/koala_tensor.md:511` 的接口表仍写 `fmt/to_str`，`to_str` 已删除，需同步。
- 实际签名是 `kl_new_tensor(int ndim, const int64_t *shape)`（无 Device 参数）、
  `kl_tensor_free(TensorObject *t)`；`tensor_data()` 是头文件里的 static inline，无需实现。

### 9. 跑测试要用新库，否则测的是旧安装

- [x] 安装流程已确认（见 §0）：`build-debug.sh` 只编不装，装要另跑 `install-debug.sh`

`bin/koala` 动态链到 `~/.local/lib/koala/lib/libkoala.so`（旧的），`build-debug.sh` 里 install 是注释掉的。
验证新代码要 `export LD_LIBRARY_PATH=$(pwd)/build/debug/lib:$LD_LIBRARY_PATH`，或跑 `install-debug.sh`。
另注意 `env.sh` 用 `$(pwd)` 拼 `KOALA_PATH`，**必须在 koala-lang 根目录 source**，否则路径全错。

### 10. 上一轮遗留

- [ ] `test_union_failed.kl` / `test_union_return_error.kl` 是否删除
- [ ] D12：`Tensor == Tensor` 是否应报错（现在走默认 identity 比较）
- [ ] issue #136

---

## 附：写 tensor 测试时踩到的 koala 易用性问题（非 tensor，建议进 `issues/`）

### 11. `list == list` 是身份比较，且断言失败信息无法区分

- [ ] 待定

`[1, 2] == [1, 2]` 返回 `false`（`a == a` 才 true）。于是 `ut.assert_eq(t.shape(), [2, 3])` 必然失败，
而且 panic 信息两边打印完全一样：`assertion failed: [2, 2] != [2, 2]`，极难排查。
要么给 list 结构化 `__eq__`，要么让 `assert_eq` 对 list 走逐元素比较 / 失败时提示是身份比较。

### 12. `str(list)` 静态被拒，但 `print(list)` 可以

- [ ] 待定

`str(t.shape())` 编译报 `Incompatible type: expected 'std/builtin.ToString', but got 'std/builtin.list[int64]'`，
而 `print(t.shape())` 正常输出 `[2, 3]`。目前只能用 `format("{}", t.shape())` 绕过
（`test/test-ut/test_tensor.kl` 就是这么写的）。list 要么补 `ToString`，要么 `str` 放宽到 any。

### 13. 工厂调用不写 `[float32]` 也不写标注时，推不出元素类型

- [ ] 待定方案（迁包后 B 已出局：编译器里不再有任何 Tensor 特判）

现状（迁包后实测）：

| 写法 | 结果 |
| --- | --- |
| `let t Tensor[float32] = Tensor.zeros(2, 3)` | ✅ 从标注推 U |
| `let t = Tensor.zeros[float32](2, 3)` | ✅ |
| `let t = Tensor.zeros(2, 3)` | ❌ `cannot infer type parameter 'U' for function 'zeros'` |
| `print(Tensor.zeros(2, 3))` | ❌ 同上 |
| `let t Tensor = Tensor.zeros(2, 3)` | ❌ `'Tensor' is not found`（关键字已删，裸 `Tensor` 不再等于 `Tensor[float32]`） |

7 个工厂带 `let t Tensor[float32] =` 标注都正常，测试见 `test_tensor_infer_from_annotation`
（原 `test_tensor_default_dtype`，迁包后改名）。

根因不变：`type_param_decl` 只有 `ID` / `ID : klass_list` / `INFER ID`，
**语言没有默认类型实参**（写不了 `[U = float32]`）；`parser_generic.c` 推不出来时只回退到
`infer_tp_from_expected`（看标注/期望类型），没有期望类型就报错。

候选方案：

- A：语言级支持默认类型实参 `[U = float32]`（语法 + `TypeParamDecl` + 推断回退）。通用，改动最大；
  与「编译器不认识 Tensor」一致——默认值写在 `torch/tensor.kl` 里，不在编译器里。
- ~~B：在推断回退里对 `Tensor` 的元素类型参数特判默认 float32~~ **已出局**：本轮已把 Tensor 的
  全部编译器特判删除，再加硬编码与该决定冲突。
- C：维持现状，必须写 `let t Tensor[float32] = ...` 或 `Tensor.zeros[float32](...)`，文档写清楚。
- D：包级门面函数（`torch.zeros(...)` 直接返回 `Tensor[float32]`），把默认 dtype 放在库里而不是语言里，
  与文档规划的 `koala/np` 门面同路。
