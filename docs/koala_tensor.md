# Koala Tensor 设计（定稿）

本文是 Koala `Tensor` 类型的定稿设计，面向 AI 算子开发。参考 PyTorch 和 NumPy：吸收好的设计，去掉公认的坏设计。

- 本文**取代** `docs/ndarray.md`，以及 `docs/koala_ml.md` §4.2、§4.3、§4.5、§13.2 中与本文冲突的结论；`docs/koala_pytorch.md` 的图编译路线以本文 §9 为准。
- 当前状态：**设计定稿，尚未实现**。实现按 §11 的版本路线逐版推进。
- 文档结构：§1 决策总表 → §2–§9 总体设计（所有版本共用的最终形态）→ §10 版本总览 → §11 各版本支持范围、详细方案和步骤 → §12 风险 → §13 PyTorch/NumPy 迁移对照。

---

## 1. 决策总表

| # | 项目 | 结论 | 详见 |
|---|---|---|---|
| D1 | dtype | 静态类型参数 `Tensor[T]`；不写 T 的 `Tensor` 默认 `float32`；`koala/np` 门面默认 `float64` | §2.1 |
| D2 | rank/shape | **没有任何语法**（不进入类型，也没有注解）；由编译器自动推导，推导出来就优化，推导不出来就不优化；常量 Tensor 一定能推导出来；不报编译期形状错误，只给 warning | §2.2 |
| D3 | 0 维 Tensor | 支持，语义上就是标量；`.item()` 取得 host 标量；索引和归约一律返回 Tensor | §2.3 |
| D4 | 类型提升 | 按 PyTorch/NumPy 2（NEP 50）：标量是弱类型，整数做 `/` 得到浮点，原地运算不能把结果降到更低的类别 | §3 |
| D5 | Storage | 由 GC 管理：新增 `GC_KIND_STORAGE`，有独立接口和每个设备独立的阈值；数据区由设备分配器分配；不用 refcnt，用 `SHARED` 标志位；现阶段只能手动 GC，预留自动 GC 的钩子 | §4 |
| D6 | stride | 禁止负 stride；允许 stride 为 0（广播和 expand 需要）；禁止写入内存重叠的 view | §5 |
| D7 | 设备 | `DeviceBackend` vtable 加流/事件语义一次定好；V1 只实现 CPU；device 是运行时字段，不进入类型 | §6 |
| D8 | view、原地修改、算子注册、autograd | 按 PyTorch：ATen schema、dispatcher、Meta 后端、custom_op、version counter；高阶导数放到 V6 | §5、§7、§8 |
| D9 | 优化 | KLR 层的轻量优化（L1）加 TIR 图优化（G1–G6）；以编译期静态抓图为主；图内用 arena 做内存规划；后端 B1 → B2/B3 → 设备 | §9 |
| D10 | 常量 Tensor | 编译期常量进入 klc 常量池（perm storage）；运行期用 `READONLY` 标志；推理时由 freeze pass 把权重当图常量。不提供类型级的 `const` | §2.4 |
| D11 | `Array` 语法 | 删除 `koala.y` 中的 `array_type` 和 `ARRAY` 关键字（目前无人使用），数值数组统一用 `Tensor[T]` | §2.2 |
| D12 | 比较运算 | **方案 A**：逐元素比较只用方法 `eq ne lt le gt ge`，返回 `Tensor[bool]`；`equal(b) bool` 判断整体是否相等。Tensor 不重载 `==` 和 `<`：源码里对两个 Tensor 写 `==`/`!=`/`<` 等运算符时编译期报错，提示改用对应方法或 `is`。运行时的自动 Equatable 按对象同一性比较，供容器使用 | §3.1 |

---

## 2. 类型系统

### 2.1 dtype：静态的 `Tensor[T]`

- `T` 必须满足 `DType` 约束。可选的类型有：`bool`、`int8/16/32/64`、`uint8/16/32/64`、`float16`、`bfloat16`、`float32`、`float64`。复数（`complex64/128`）和 fp8（`float8_e4m3/e5m2`）在 V6 加入。
- 不允许 `Tensor[str]` 或 `Tensor[Object]`。Storage 里不存放任何对象引用，这是 Storage 回收时可以不扫描数据区的前提（§4）。
- 不写 T 的 `Tensor` 等价于 `Tensor[float32]`。如果泛型默认参数做不到（由 V0 的 spike 验证），退路是：在 `std/builtin` 里加 `type Tensor = Tensor[float32]` 这样的别名，或者要求显式写 T。
- 量化张量单独设计为 `QTensor[Q]`（V6）：数据按块打包，附带 scale 和 zero 等参数，不按单个元素寻址，所以不放进 strides 体系。
- 运行时同样保存 dtype（`TensorObject.dtype`），供 `DynTensor`、序列化、分发和调试使用。编译期已知 T 的地方，运行时不再检查 dtype。
- `DynTensor`：擦除了 dtype 的 Tensor，用在 IO 边界，例如 `load("x.safetensors") -> dict[str, DynTensor]`。通过 `t.as[float32]()`（运行时检查）转换回静态类型（V2）。

### 2.2 rank 和 shape：没有语法，由编译器自动推导

**原则**：rank 和 shape **不需要任何语法**，没有类型参数、形状注解或断言方法。用户只写 `Tensor[T]`。编译器在优化阶段自动推导形状：**推导出来的就优化，推导不出来的就不优化**，语义完全由运行时保证。

```koala
func f(x Tensor[float32]) Tensor[float32] { ... }   # 唯一的写法
let w = Tensor.zeros(784, 10)                       # 编译器推导出 [784, 10]
let c = Tensor.from_list([1.0, 2.0, 3.0, 4.0], 2, 2) # 常量 Tensor：形状和值都已知
```

**形状推导**（V2 的 L1 层和 V3 的 TIR 形状推断使用同一套规则）：

| 情况 | 推导结果 | 编译器能做什么 |
|---|---|---|
| 常量 Tensor（§2.4 C1） | rank、shape 和值全部已知 | 常量折叠，写入 klc 常量池，消费它的算子可以特化 |
| 创建函数的参数是常量或可推导，例如 `zeros(3, 4)`、`arange(0, n)` 中 n 是常量 | rank 和 shape 已知 | 去掉形状检查，算出步长常量，展开循环，`at()` 变成直接地址计算 |
| 由已知形状经算子推出，例如 view、广播、归约、matmul（借助 Meta 形状函数） | 沿数据流传播 | 同上；融合时可以静态确定迭代空间 |
| 部分已知，例如 rank 已知但某一维未知 | 已知的部分照常使用 | 按已知部分做特化，未知维保持动态 |
| 函数参数、来自 IO 的数据、依赖数据的算子（`nonzero`、`reshape(变量)` 等） | 未知 | 不优化，走通用的运行时路径 |
| 内联之后 | 调用点上实参的形状流入被内联的函数体 | 内联能扩大可推导的范围 |

**规则**：

- **不报编译期形状错误**：即使推导发现必然不匹配，比如 `zeros(3,4).matmul(zeros(5,6))`，也只给出 **warning**，不报错，语义仍由运行时 panic 保证。这样，有没有优化、优化到什么程度，都不会改变程序是否能编译通过。
- **不影响类型**：形状只是编译器的分析结果，不进入 TypeSpec，不参与类型检查、重载、单态化、泛型匹配和 itable。
- **运行时总是携带完整的 shape/strides**：不依赖编译期推导。V3 图编译对推导不出来的形状，使用"按运行时具体形状特化加守卫"（§9.4 G3），这同样不需要语法。
- **可观察性**：`--dump=tensor-shape` 打印每个 Tensor 值推导出来的形状（已知、部分已知、未知），方便查看为什么某处没有被优化。

**为什么不强制 rank/shape 进入类型**（讨论结论）：

1. 一些常用算子的输出 rank 由运行时数据决定，静态类型写不出来：`squeeze()`、`reshape(shape)`、`nonzero/unique`、`a[mask]`、参数为变量的 `sum(axis=ax)`、`einsum`、从文件加载。
2. 写通用代码需要 rank 多态，也就是 const generics 和变长维度。Koala 泛型目前两者都没有：`koala.y:1144` 的 shapes 被丢弃了，TypeSpec 中也没有整数类型参数。
3. 广播和 conv 输出尺寸需要在类型层做整数运算，接近依赖类型，类型检查器的复杂度会上一个数量级。
4. 每种 rank 和形状都会触发一次单态化（`specialize.c` 按实例克隆 IR），代码膨胀，而内核本来就是 native 的，没有收益。
5. 变长序列、KV cache、最后一个不满的 batch 本来就是动态形状，强制写进类型没有收益；PyTorch 代码移植时还要逐个补标注。
6. 可选注解也不要：编译器能推导出来的，注解是多余的；推导不出来的，注解只能变成运行时守卫，没有优化价值，反而增加语法和学习成本。

**`Array` 语法**：`koala.y` 里的 `array_type`（`Array[T, d...]`、`[d...]T`）和 `ARRAY` 关键字一起**删除**，不迁移成 Tensor 的形状语法。全仓库都没有用到，shapes 也一直被丢弃（`koala.y:1144`）。需要定长数值数组时，直接用 `Tensor[T]`。

**维度的规格**：每个维度用 int64；`numel` 也是 int64；允许 0 维，允许大小为 0 的维；支持负数维度索引；`reshape` 支持 `-1`。前 6 维的 shape 和 strides 内联存放，超过 6 维时放到堆上；最多 64 维（与 NumPy 2 相同）。

### 2.3 0 维 Tensor 与 host 标量

| 规则 | 说明 |
|---|---|
| 0 维 Tensor 和 host 标量是两个类型 | `Tensor[float32]` 且 shape 为 `()`，可以在 GPU 上、可以带 grad；`float32` 是寄存器值。不合并，以免普通标量也背上 Storage |
| 索引和归约的返回值 | `a[i, j]`、`sum()`、`mean()` 一律返回 Tensor（全局归约返回 0 维），返回类型不随实参变化 |
| 0 维 → 标量 | `.item() -> T`。这是显式同步点，也是抓图时的 graph break |
| 快速标量访问 | `a.at(i, j) -> T`。编译器会把它，以及静态 rank 下的 `a[i, j].item()`，折叠成 `OP_TENSOR_LOAD` |
| 标量 → 0 维 | `Tensor[T].scalar(x)`；在运算中标量会自动作为弱标量参与（§3） |
| 不能当条件 | `if t`、`if a.lt(b)`（结果是 `Tensor[bool]`）在编译期报错，需写 `.item()`、`.any()` 或 `.all()` |

### 2.4 常量 Tensor

| 层次 | 来源 | 处理 |
|---|---|---|
| C1 编译期常量 | `Tensor.from_list` 参数全是字面量；`zeros/ones/eye/arange/full` 参数全是常量 | 常量折叠后序列化进 klc 常量池；运行时用 `gc_alloc_perm_storage`，并标记 `READONLY \| PERM`。大于 64KB 的不内联，改为运行时构造 |
| C2 运行期只读 | 以 mmap 方式加载的权重、`t.freeze()` | Storage 标记 `READONLY`，写入时 panic（消息里带算子名和形状）；view 继承这个标志 |
| C3 图常量 | 推理模式下用 `freeze(model)` | TIR 的 freeze pass 把权重提升为图常量，从而可以做常量折叠、权重预打包、BN 折叠 |

- **C1 的形状一定能推导出来**（§2.2）：rank、shape 和 dtype 都在编译期已知。即使数据超过 64KB、不写入 klc，改在运行时构造，形状信息也保留，消费它的算子照样能特化。
- C2 和 C3 的形状在运行时才知道（来自文件或模型），编译器不做假设；V3 的图编译按运行时的具体形状特化，并加守卫。

`let` 只约束变量绑定不能改，不约束数据是否可写；这一点在手册中写明。

---

## 3. 类型提升（PyTorch / NumPy 2）

T 是静态的，所以结果类型**在编译期算出**，算法与 `torch.result_type` 相同。

| 规则 | 内容 | 示例 |
|---|---|---|
| P1 类别 | 由低到高：bool < 整数 < 浮点 < 复数 | — |
| P2 Tensor 与 Tensor | 类别不同时取较高的类别；类别相同时取较宽的类型；有符号和无符号混合时取能同时容纳两者的有符号类型 | f32+f32→f32；f16+f32→f32；bf16+f16→f32；int32+f32→f32；uint8+int8→int16；int64+uint64→int64（与 PyTorch 一致） |
| P3 弱标量（字面量或标量变量） | 与 Tensor 同类别时不改变 dtype；类别更高时提升到该类别的默认类型 | `lr(f64) * t_f32`→f32；`2 * t_int8`→int8；`0.5 * t_int64`→f32 |
| P4 0 维 Tensor | 与弱标量同等对待（与 PyTorch 一致） | `t_f32 * zero_dim_f64`→f32 |
| P5 真除法 `/` | 整数或 bool 输入时，结果为默认浮点类型 f32 | `Tensor[int64] / 2`→f32 |
| P6 地板除 | 用 `floor_divide()`，或者语言里已有的整除运算符，结果保持整数 | — |
| P7 原地运算和 `out=` | 结果不能降到更低的类别（编译期报错）；同类别时截回 T | `t_int += 0.5` 报错；`t_f16 += t_f32` 允许 |
| P8 比较方法 | `eq/ne/lt/le/gt/ge` 先按 P2/P3 对齐两侧，结果是 `Tensor[bool]`；不使用运算符形式（D12、§3.1） | `x.lt(0.0)`、`pred.eq(y)` |
| P9 归约 | 对整数或 bool 做 `sum/prod/cumsum` 时提升到 int64；对整数做 `mean` 报错；对 f16/bf16 内部用 f32 累加、输出 T | 与 PyTorch 一致 |
| P10 显式转换 | `to[T]()` 或 `to(dtype)`，dtype 不同时总是拷贝，相同时返回自身 | — |
| P11 整数溢出 | 按 T 的宽度回绕（与 PyTorch 一致） | — |
| P12 float 转 int | 与运行时 cast 的现有语义一致（饱和），写入手册 | — |

- 默认浮点类型固定为 f32；**不提供**全局可变的 `set_default_dtype`。
- 实现方式：Tensor 的运算符在 `tensor.kl` 中声明为 `@intrinsic`（只能在 `std/builtin` 中使用，`parser.c:1461`）；类型检查器新增 `tensor_result_type(T, U, op)`；在 `parse_binary` 中新增"标量在左、rhs 为 Tensor"的分支。

### 3.1 比较运算（D12，方案 A）

- **逐元素比较用方法**：`eq ne lt le gt ge`，Tensor 或弱标量都可以作参数，按 P2/P3 对齐后返回 `Tensor[bool]`。
- **整体比较**：`equal(b) bool`（形状和元素都相等）、`allclose(b, rtol, atol) bool`。
- **不重载运算符**：两个 Tensor 之间写 `==`、`!=`、`<`、`<=`、`>`、`>=` 都在编译期报错，例如 `` `==` on Tensor compares elementwise in PyTorch; use a.eq(b) / a.equal(b), or `is` for identity ``。Tensor 不实现 `Comparable`。
- **为什么不重载**：每个类都会自动获得 `Equatable`（`parser_expr.c:1743`），而且 `__eq__` 约定返回 `bool`（`any.kl:30`）。泛型 `T` 上的 `==` 会直接降为 `OP_NUM_EQ → kl_call_eq`，并把结果当作 bool（`parser_expr.c:4660`）；`list` 的 contains/index 也同样依赖 `SLOT_EQ`（`listobj.c:81`）。如果 Tensor 的 `==` 返回 Tensor，会破坏容器和泛型。另外，NumPy/PyTorch 中"`==` 返回数组"本身也被公认为坏设计：会出现 truth value ambiguous 错误，`in` 判断出错，也无法作为 dict key。
- **运行时语义**：Tensor 的 eq/hash slot 按对象同一性实现。所以 `list[Tensor]` 的 `in`/`index`、dict key、泛型代码中的 `==` 都是同一性比较，与 Python 对象默认行为一致。
- **写法示例**：

```koala
x.masked_fill_(x.lt(0.0), 0.0)            # x[x < 0] = 0
let y = where(x.gt(0.0), x, x * 0.1)      # leaky relu
let acc = pred.eq(y).to[float32]().mean() # 准确率
```

---

## 4. 运行时结构与内存（Tensor 与 Storage 分离，都由 GC 管理）

### 4.1 结构

```
TensorObject  (GC_KIND_OBJECT)                StorageObject (GC_KIND_STORAGE)        数据区
  OBJECT_HEAD                                   StorageHeader:                       (不在 GC 堆)
  storage*  ─────────── gc_mark ──────────▶      data* ───────────────────────────▶  设备分配器
  dtype (uint8) / device (Device)                nbytes (size_t) / align              CPU: 缓存分配器
  ndim / offset (元素)                            device (含 driver*)                  GPU/NPU: 设备分配器
  shape[6] / strides[6] (内联, 元素为单位)        flags: READONLY|PERM|MMAP|            mmap/DLPack: 外部
  ext_dims* (ndim > 6)                                   EXTERNAL|SHARED|PINNED
  flags: contiguous / channels_last 缓存          version (uint64, 原地写 +1)
  autograd_meta* (惰性分配, GC 对象)              ready_event* (异步设备)
                                                 deleter (外部内存)
```

- 不复用现有的 `Storage`（`object.h:57`，那是 tuple/list 的 TValue 块）。Tensor 的存储另起名为 `StorageObject`，避免混淆。
- strides 和 `offset` 都以元素为单位。
- `autograd_meta` 只在需要时才分配，推理路径上的 Tensor 头保持很小。

### 4.2 GC 新增接口（与现有接口并列，不改现有接口）

| API | 说明 |
|---|---|
| `GC_KIND_STORAGE` | 新的块类型。mark 时和 BYTES 一样直接标黑，不扫描数据区；sweep 时调用 `mem_ops->free` 或 `deleter` |
| `StorageObject *gc_alloc_storage(size_t nbytes, size_t align, Device dev)` | 头部是一个小 GC 块；数据区由 `mem_ops->alloc` 分配，大小类型为 `size_t`，不受 `gc_alloc(int)` 和 `KOALA_GC_MAX` 的限制 |
| `gc_alloc_perm_storage(...)` | 给常量 Tensor 用，永不回收 |
| `gc_wrap_storage(data, nbytes, dev, deleter)` | 接管外部内存（mmap、safetensors、DLPack） |
| `gc_free_storage(StorageObject *)` | 显式提前释放，只在 §4.3 的条件下调用 |
| `gc_storage_used(dev)` / `gc_storage_max(dev)` | 每个设备单独计数、单独设阈值。CPU 用 `KOALA_STORAGE_MAX`（默认 4G），后续加 `KOALA_CUDA<n>_MAX` |
| `GCStats` / `GCCycleStats` 新增字段 | `storage_alive`、`storage_bytes`、`storage_freed`，`std/gc` 模块同步暴露 |

**超过阈值时（现阶段的行为）**：先调用 `mem_ops->empty_cache()` 再试一次；仍然超过就 panic，报错信息包含设备、申请的字节数、已用字节数和阈值，并提示"调用 `gc.collect()`，或使用 `free` / `tensor_scope`"。

`gc_alloc_storage` 里预留一个 `gc_storage_pressure_hook`，留给以后的自动 GC。这个位置本来就是一次 helper 调用，符合 JIT `DESIGN.md` §14 对 GC 点的约定。

**sweep 与异步设备**：如果一个 STORAGE 块的 `ready_event` 还没完成，sweep 时不立即释放，而是挂到 `pending_storage` 队列，等下一次 `gc_alloc_storage` 或下一次 GC 时再检查。CPU 上事件总是已完成，不会走到这条路径。

### 4.3 不用 refcnt，改用 `SHARED` 标志

- **规则**：同一个 Storage 上只要出现第二个 Tensor 头，就置 `SHARED`，并且永不清除。产生第二个头的情况包括：任何 view、`detach()`、`set_()`、autograd 把它当别名保存。这个判断偏保守，代价只是一条写指令。
- **提前释放**统一走 `tensor_free(t)`。显式 `free(t)`、编译器自动插入的 free、`tensor_scope` 都调用它，步骤如下：
  1. 如果 Storage 没有 `SHARED`，也不是 `READONLY`/`PERM`/`EXTERNAL`，就调用 `gc_free_storage`；
  2. 调用 `gc_free(头)`。注意 `gc_free()` 不会调用 `_free` 钩子（`gc.c:179`），所以第 1 步只能由 `tensor_free` 自己做，写法参照 `strobj.c:1724`。
- **原地复用**：把 `y = a + b` 改写成 `a += b`，需要同时满足三点：
  - 编译期能证明 `a` 之后不再使用；
  - 运行期检查 `a` 的 Storage 没有 `SHARED`；
  - autograd 不需要保存 `a`。

### 4.4 只能手动 GC 时如何控制内存（按优先级）

1. 编译器逃逸分析：循环里没有逃逸的临时 Tensor，自动插入 `tensor_free`（V2）。
2. TIR 融合和 arena 内存规划：中间结果根本不单独分配（V3）。
3. `with tensor_scope() { ... }`：作用域结束时，释放作用域内新建且没有逃逸的 Tensor（V1 先提供运行时实现）。
4. autograd：`backward()` 在 `retain_graph=false` 时，执行完就清除每个 Node 保存的张量引用（V4）。
5. 用户手动调用 `gc.collect()`；以后可以启用自动 GC 钩子。

### 4.5 CPU 分配器

- 按 2 的幂分档的缓存分配器，64B 对齐。2MB 及以上的块用 `mmap` 分配，并尝试 `MADV_HUGEPAGE`。
- 提供 `empty_cache()` 和 `memory_stats()`（allocated / reserved / peak / num_alloc）。这就是 `DeviceBackend.alloc/free` 在 CPU 上的实现。
- 分配器内部加锁。内核以后会用多线程执行，但分配只发生在 VM 线程。

---

## 5. view 与原地修改（按 PyTorch）

| 项目 | 规则 |
|---|---|
| 返回 view | `view`、基本索引 `t[...]`、`narrow`、`select`、`unsqueeze`、`squeeze`、`transpose`、`permute`、`expand`、`unfold`、`split/chunk/unbind`、`diagonal`、`as_strided`（仅内部使用，不对外开放） |
| 返回拷贝 | 高级索引（整数 Tensor 或 bool 掩码）、`flip`、`contiguous()`（本来就连续时返回自身）、dtype 不同时的 `to[T]()` |
| reshape | 能做成 view 就返回 view，否则拷贝（与 PyTorch 一致）。如果代码写入 reshape 的结果，编译器给出警告。`view()` 必须共享内存，做不到就报错 |
| 负 stride | 禁止。步长是字面量时，`a[::-1]` 在编译期报错；是变量时在运行期 panic，提示改用 `flip(dim)` |
| stride 为 0 | 允许（`expand` 和广播需要）。但向这类 view 写入，或者向内存重叠的 view 写入，都会报错 |
| 原地运算命名 | 方法以 `_` 结尾，写入 `self` 并返回 `self`：`zero_`、`fill_`、`copy_`、`clamp_`。原地四则运算**只用运算符** `+= -= *= /=`（`__iadd__` 等），不提供 PyTorch 的 `add_/sub_/mul_/div_`，避免同一件事两种写法 |
| `t[idx] = v` | 基本索引时等价于 `select/narrow(...).copy_(v)`；整数或掩码索引时走 `index_put_` |
| 内存重叠 | 原地运算会检查输入和输出是否部分重叠（如 `a += a.transpose()`），部分重叠就报错；完全相同的内存允许 |
| version counter | `StorageObject.version` 被同一 Storage 上的所有 view 共享，每次原地写加 1。autograd 的 SavedVariable 靠它检测数据是否被改过 |
| view 与 autograd | view 记录 `_base`。对 view 做原地修改时，同时重写 base 的 grad_fn（PyTorch 的 `CopySlices` 机制） |
| 只读 | Storage 带 `READONLY` 时，任何原地写都会 panic |
| 内省 | `is_contiguous()`、`data_ptr()`、`storage_offset()`、`untyped_storage()`、`stride()` |

---

## 6. 设备架构（V1 定好接口，只实现 CPU）

### 6.1 分层

```
Koala 层   Tensor[T]  .to(dev)  .device  with device_scope(dev) { }
运行时     TensorObject ──▶ StorageObject(GC_KIND_STORAGE)
分发层     Dispatcher: op × DispatchKey × dtype → kernel  (+ Meta 形状函数 + autograd)
设备层     DeviceDriver: dev_ops / mem_ops / copy_ops / sync_ops / exec_ops
             ├─ cpu            V1
             ├─ cuda / rocm    V5
             ├─ npu (ascend)   V5+
             └─ metal / vulkan 远期
```

### 6.2 设备 ops 表（V1 冻结）

仿 Linux VFS：按「谁持有」拆表，而不是把设备的函数指针塞进一张大表（`super_block` 挂 `s_op`、`inode` 挂 `i_op`/`i_fop`、`address_space` 挂 `a_ops`）。`DeviceDriver` 像 `net_device` 挂 `netdev_ops` 那样挂着几张 ops 指针，不支持的整张留 NULL。

| ops 表 | 持有者 | 接口 | CPU 实现 |
|---|---|---|---|
| `DeviceOps` | 设备本身 | `count()`、`name(i)`、`caps(i)`（dtype 支持、对齐、是否统一内存、是否只支持图模式） | 1 个设备，64B 对齐，支持全部 dtype |
| `DeviceMemOps` | Device（Storage 经 `device.driver` 取用） | `alloc(nbytes, align, stream)`、`free(ptr, stream)`、`empty_cache()`、`stats()` | V1-min 用 `aligned_alloc(64)`，缓存分配器（§4.5）后续 |
| `DeviceCopyOps` | 搬运路径 | `copy(dst, src, nbytes, kind, stream)`，`kind` 取 h2d / d2h / d2d / h2h | `memcpy` |
| `DeviceSyncOps` | Stream / Event | `stream_create()`、`stream_sync(s)`、`default_stream()`、`event_record(s)`、`event_query(e)`、`event_sync(e)` | 空实现（同步执行），事件立即完成 |
| `DeviceExecOps` | 内核 / 图 | `launch(kernel, args, stream)`；`graph_compile(TIR)`、`graph_run(g, inputs, outputs, stream)`（可选） | launch 直接调用，内部可用线程池；图由 B1 后端提供 |

`copy` 单独一张表：PyTorch 里 `copy_` 是走 dispatcher 的普通算子（§7），dispatcher 落地后这张表可能整体退场，不与内存 ops 绑死。

Storage 只持有 `Device`，`Device` 持有 `const DeviceDriver *`，分配和释放走 `device.driver->mem_ops`；Storage 自己不持任何 ops 表，也看不到流、事件和内核。多一层间接，对应 `inode->i_mapping->a_ops`。`Device` 由 `device_make(kind, index)` 构造并填好 driver，未编入的后端 driver 为 NULL；相等只看 `kind + index`。

### 6.3 为 GPU/NPU 预先定好的约定

| 问题 | 约定 |
|---|---|
| 异步释放 | 每次 launch 后，在输入和输出的 Storage 上记录 `ready_event`；sweep 时事件未完成的 Storage 放进 `pending_storage` 延后释放（§4.2） |
| 同步点 | 只有 `.item()`、`.tolist()`、`.to(cpu)`、打印、`synchronize()` 会同步；与抓图的 graph break 规则统一维护 |
| 跨设备运算 | 运行时报错（如 `add: tensors on cpu and cuda:0`），不会静默搬运数据 |
| 当前设备 | 用 `with device_scope(dev)` 指定（基于 `Withable`），不使用全局可变状态；工厂函数默认使用当前作用域的设备 |
| 设备预算 | 每个设备一个 `gc_storage_max(dev)` |
| dtype 能力 | 由 `caps` 声明；不支持的组合在分发时报错，不静默降级 |
| NPU | NPU 偏好静态形状和整图执行，因此 `graph_compile` 是一等接口；eager 模式下每个算子作为单算子图执行 |
| 后端加载 | 后端以 NativeLib 动态库形式提供（`kl_reg_*`），`import std/cuda` 时注册 vtable 和内核；核心运行时不链接任何 GPU 库 |
| 互操作 | `from_dlpack` / `to_dlpack`，配合 `gc_wrap_storage(deleter)` |
| 语言层 | `Device` 是值类型 `{kind, index, driver*}`，C 侧用 `device_make(kind, index)` 构造，语言侧用 `cpu()`、`cuda(0)`、`npu(0)`；同设备时 `.to(dev)` 返回自身；`pin_memory()`；`non_blocking=true` |

---

## 7. 算子注册与分发（按 PyTorch）

### 7.1 schema（ATen 写法）

```
"add.Tensor(Tensor self, Tensor other, *, Scalar alpha=1) -> Tensor"
"add_.Tensor(Tensor(a!) self, Tensor other, *, Scalar alpha=1) -> Tensor(a!)"
"view(Tensor(a) self, SymInt[] size) -> Tensor(a)"
```

`(a!)` 表示原地修改该参数，`(a)` 表示返回值与该参数共享内存（别名）。functionalize 和 TIR 的别名分析直接读取这些标注。

### 7.2 分发键（优先级从高到低）

`ModeHook → Autocast → Autograd → Functionalize → ADInplaceOrView → Backend(CPU | CUDA | NPU | Meta)`

- 每个 Tensor 带一个 `DispatchKeySet`，由 device、requires_grad 和当前模式推导；分发时取最高位对应的键，查表 O(1)。
- **Meta 后端**只计算形状、dtype 和 strides，不分配数据。TIR 的形状推断、守卫和编译期形状检查都用它。

### 7.3 三类内核

| 类型 | 说明 | 例 |
|---|---|---|
| 原生 | C 实现，按设备和 dtype 注册 | add、matmul、conv2d |
| CompositeImplicit | 由其他算子组合实现，自动获得 autograd 和所有设备支持 | linear、softmax 的参考实现 |
| CompositeExplicit | 组合实现加手写反向 | log_softmax、layer_norm |

### 7.4 Koala 侧自定义算子（对应 `torch.library.custom_op`）

```koala
@op("myops::rms_norm(Tensor x, Tensor w, float eps) -> Tensor", kind=opaque)
func rms_norm(x Tensor, w Tensor, eps float32) Tensor { ... }   # 参考实现

@rms_norm.register_kernel(cpu)            # native 或 Koala 写的快速内核
@rms_norm.register_fake                   # Meta 形状函数
@rms_norm.register_autograd(backward, setup_context)
```

- `kind` 取 `pointwise`、`reduction` 或 `opaque`，决定该算子能否参与融合。
- C 扩展通过 NativeLib 注册内核。
- `opcheck(op, samples)` 会检查：schema 与别名标注是否一致、Meta 推出的形状与真实内核是否一致、`gradcheck`、eager 与 compiled 结果是否一致。

---

## 8. autograd（按 PyTorch）

| 项目 | 设计 |
|---|---|
| 叶子 | `requires_grad`、`.grad`（梯度累加）、`is_leaf`、`retain_grad()`、`requires_grad_()` |
| 图 | 非叶子张量的 `grad_fn` 指向一个 Node（GC 对象），Node 通过 `next_functions` 连向上游；Tensor 与 Node 之间的环由 GC 回收 |
| 保存张量 | SavedVariable 记录保存时的 version；反向时 version 不一致就报错 "modified by an inplace operation" |
| 引擎 | `backward(gradient=nil, retain_graph=false)`、`autograd.grad(outputs, inputs)`；按依赖计数做拓扑序执行；V4 单线程 |
| 作用域 | `with no_grad()`、`with enable_grad()`、`with inference_mode()`（基于 Withable）；inference_mode 下产生的张量不带 version 和 autograd 元数据 |
| 自定义 | `class F : autograd.Function`，实现 `forward(ctx, ...)`、`backward(ctx, grads...)`，用 `ctx.save_for_backward` 保存张量 |
| 钩子 | `register_hook`、`register_post_accumulate_grad_hook` |
| 内存 | `retain_graph=false` 时，反向结束后清除 SavedVariable 的引用 |
| 工具 | `gradcheck`、`detect_anomaly()` |
| 后置 | 高阶导数（`create_graph=true`）、`torch.func` 风格的 `grad/vjp/jvp/vmap`（V6） |

---

## 9. 优化器与算子图

### 9.1 两层结构

```
KLR（带类型的 SSA）
  ├─ L1 Tensor 感知的轻量优化（eager 模式下也生效）
  └─ 抓图 → TIR（算子图，纯数据流 DAG）
            ├─ G1 规范化
            ├─ G2 functionalize（依据 schema 的别名标注）
            ├─ G3 形状推断（Meta 后端）+ 守卫
            ├─ G4 融合 + 调度
            ├─ G5 内存规划（arena）
            └─ G6 后端：B1 分块解释 / B2 JIT SIMD / B3 C 代码生成 / DeviceBackend.graph_compile
```

### 9.2 L1：KLR 层

| Pass | 内容 | 版本 |
|---|---|---|
| 专用 opcode | `OP_TENSOR_BINOP/UNOP/REDUCE/LOAD/STORE`，替换 `OP_NUM_*` 加 slot 分发的路径 | V2 |
| dtype/形状推导 | 结果 dtype 在编译期按 §3 算出；按 §2.2 自动推导形状（常量 Tensor、创建函数、Meta 形状函数传播）；推导出必然不匹配时给 warning | V2 |
| 标量访问折叠 | `at()` 以及静态 rank 下的 `[..].item()` 折叠成 `OP_TENSOR_LOAD`；JIT 内联地址计算；循环不变的 stride 和边界检查提到循环外 | V2 |
| 逃逸分析与自动 free | 没有逃逸的临时 Tensor 由编译器插入 `tensor_free`（运行时会检查 `SHARED`） | V2 |
| 原地化改写 | 满足 §4.3 的条件时，把 `y = a op b` 改写为 `a.op_(b)` | V2 |
| 常量 Tensor | 常量折叠后写入 klc 常量池，运行时使用 perm storage | V2 |
| 新增文件 | `src/parser/opt/tensor_l1.c`，不改动已有的 pass | — |

### 9.3 抓图：编译期静态提取

- 在 KLR 上，按 `type_is_tensor` 找出由 Tensor 值组成的最大区域；先内联（`opt/inline.c`）把区域扩大，再降成 TIR，最后替换为一条 `OP_TENSOR_GRAPH`。
- **graph break 规则**（与 §6.3 的同步点合并）：
  - `.item()`、`.tolist()`、`.to(cpu)`、打印；
  - 调用没有内联的非 Tensor 函数；
  - 副作用：IO、写全局变量、写对象字段；
  - 依赖 Tensor 值的控制流；
  - 协程挂起点；
  - 自定义算子既是 `opaque` 又没有 Meta 函数。
- V3 只在基本块内抓图；V6 加 `@compile`，跨基本块抓图，把结构化的 `if/while` 降成 `cond/while_loop` 节点。
- `--dump=tensor-graph` 打印 TIR 以及每个 graph break 的原因（对应 `torch._dynamo.explain`）。
- **兜底**：守卫检查失败时退回 eager 路径，语义与 eager 完全一致。

### 9.4 G1–G3

- **G1 规范化**：常量折叠、CSE、DCE；代数化简，例如 `transpose∘transpose`、`x*1`、`x+0`，合并连续的 reshape/view，`cat` 后接 `split` 互相抵消；推理模式下把 BN 折叠进 conv/linear（V5）。
- **G2 functionalize**：依据 `(a!)/(a)` 标注，把原地修改和 view 改写成纯函数形式，并在图末尾统一写回，对应 PyTorch 的 FunctionalTensor。V3 先只处理"没有 view、没有原地修改"的区域，V4 再完整实现。
- **G3 形状推断与守卫**：在 Meta 后端上推导出每个节点的形状和 strides。
  - V3 按具体形状特化，每张图最多缓存 8 个形状变体，超过后这张图固定走 eager，并打印诊断信息。
  - V6 引入符号维，并对 batch 维按 2 的幂分桶。
  - 守卫只检查形状签名、是否连续、device、输入之间是否存在别名。dtype 是静态的，不需要检查。

### 9.5 G4：融合（参照 Inductor）

| 类别 | 规则 |
|---|---|
| pointwise | 迭代空间相同的 pointwise 任意合并；广播和 view 只改写索引表达式 |
| reduction | 前面的 pointwise 作为 prologue 并入；对同一迭代空间的多个归约合并成一次多输出归约 |
| matmul/conv | 主体不和其他算子融合；后面的 pointwise 作为 epilogue 并入，例如 `relu(x@W + b)`，以及量化场景下的反量化 + matmul |
| 多消费者节点 | 计算量不超过 3 个 pointwise 的，在每个消费者里重算一遍；否则作为分组边界 |
| 布局 | CPU 上选 NCHW 或 NHWC，并对权重预打包；设备的私有布局由 `caps` 声明 |
| 数值约定 | pointwise 融合后与 eager 逐位一致，默认不做 FMA 合并；归约允许在容限内不同；`KOALA_TENSOR_DETERMINISTIC=1` 强制按 eager 的顺序归约；`KOALA_TENSOR_FUSE=0` 关闭融合，用于二分定位问题 |

M0 示例 `err = x*w + b - y; w -= lr*(err*x).mean(); b -= lr*err.mean()`：eager 需要 6 个内核、4 个临时数组；融合后只有 **1 个多输出归约内核、0 个临时数组**，内存只需读 2 遍。

### 9.6 G5：内存规划（与 GC 的协作）

- 图内中间结果的生命周期在编译期就已知，按区间着色后排进一个 arena。每次执行图时只调用一次 `gc_alloc_storage` 拿到 arena；执行结束后 arena 立即用 `gc_free_storage` 释放，或缓存下来供下次复用。
- 图的输出是独立的 Storage，交给 GC 管理。functionalize 之后，如果能证明某个输入已经死亡且没有 `SHARED` 标志，就原地复用它作为输出。
- 效果：编译过的区域只有输出张量会给 GC 带来压力。这是现阶段只能手动 GC 时控制内存最有效的办法。

### 9.7 G6：后端

| 档位 | 原理 | 版本 |
|---|---|---|
| B1 分块解释 | 把融合后的内核表示成微操作序列，每块 256–1024 个元素，按块解释执行。可移植、可序列化进 klc，作为正确性基线 | V3 |
| B2 JIT SIMD | 扩展 `src/jit/emit.c` 生成 SSE2/AVX2 循环。内核是叶子函数，没有 GC 点，满足 JIT 的不变量 1 和 2 | V5（先用 bench 证明有必要） |
| B3 C 代码生成 | 生成 C 代码，用系统 cc 编译成 .so，按图哈希缓存 | V5 |
| graph_compile | 设备后端整图接管 | V5+ |

### 9.8 训练进图

- V4：先做 eager autograd（运行时 tape），作为正确性基线和兜底。
- V5：AOT 联合图，即在 TIR 上为每个算子登记反向规则，把前向和反向放进同一张图，一起融合、一起做内存规划。保存还是重算：v1 规则是 pointwise 的结果重算，归约和 matmul 的结果保存；之后再做 min-cut。

---

## 10. 版本总览

| 版本 | 主题 | 一句话目标 | 依赖 | 验收标志 |
|---|---|---|---|---|
| **V1-min** | 最小可用集（**先做**） | 非泛型、只支持 f32 的 nD Tensor：GC Storage、view、广播、ufunc、归约、2D matmul | — | M0 demo 收敛；ut 全过；`storage_used()==0` |
| **V0** | 语言能力验证（**不单独做**） | 实现时遇到哪一项，就逐项讨论哪一项 | — | 结论写回本文 |
| **V1** | Eager CPU 核心 | 能用的 nD Tensor：Storage/GC、view、广播、pointwise、归约、matmul、0 维 | V0 | M0 拟合直线 demo；ut 全部通过；`gc_storage_used == 0` |
| **V2** | 编译器内置 | 类型检查内置（提升规则、标量在左）、形状自动推导、删除 `Array` 语法、专用 opcode、JIT 快速路径、L1 优化 | V1 | demo 用自然写法通过；bench 相对 V1 有加速；临时 Tensor 被自动释放 |
| **V3** | 图编译 v1 | 抓图 → TIR → 融合 → arena → B1 后端 | V2 | demo 合成 1 个内核；与 eager 的差分测试一致 |
| **V4** | 训练栈 | 完整的 dispatcher 和算子注册、functionalize、eager autograd、`nn`/`optim`、自定义算子 | V3 | MLP 训练收敛；手写梯度、autograd、`gradcheck` 三者一致 |
| **V5** | 性能与设备 | AOT 联合图、B2/B3 后端、多线程内核、CUDA 后端、mmap/safetensors | V4 | 训练时编译路径比 eager 快；同一份代码在 cuda 上运行通过 |
| **V6** | 高级特性 | 符号维、`@compile` 跨基本块抓图、高阶导数、`torch.func`、量化、NPU、fp8/复数 | V5 | 最后一个不满 batch 不触发重编译；量化 LLM 能推理 |

**通用规则**（每个版本、每个步骤都遵守，来自 `.clinerules`）：

- 每一步结束都要跑通 `source env.sh && ./build-debug.sh && ./test-debug.sh`；涉及库代码时还要跑 `./build-std-koala.sh`；耗时命令放到后台执行（`nohup ... > /tmp/xxx.log 2>&1 &`）。
- JIT 模式和 `--no-jit` 模式都要通过。
- 遇到崩溃、断言失败、可疑行为时立即停下，在 `issues/` 中记录（现象、最小复现、建议的测试用例、修复策略），等待指示。
- 不使用三目运算符，遵守现有代码风格。
- 每个步骤完成后，在本文 §14 的变更记录中更新状态。

---

## 11. 各版本详细方案与步骤

### V0 — 语言能力验证

**目标**：只回答问题，不写 Tensor 代码。每个问题都要得出"可行"或"不可行 + 退路"的结论。

| # | 验证项 | 影响 | 不可行时的退路 |
|---|---|---|---|
| V0.1 | 泛型类能否有默认类型参数（`Tensor` ≡ `Tensor[float32]`） | D1 的写法 | 类型别名；或要求显式写 T |
| ~~V0.2~~ | ~~`__eq__/__lt__` 返回非 bool~~ | 已决：采用方案 A（D12），不需要验证 | — |
| V0.3 | 禁止 Tensor 作为条件：类型检查器能否在 `if/while/&&/||` 上拦截 | §2.3 | 运行时 panic |
| V0.4 | 变长 `__getitem__(idx ...)` 混合 int/slice/Tensor/`nil`(newaxis)/`...` 时的类型推断 | 索引 API | 提供 `index(...)` 方法 |
| V0.5 | native 方法的关键字参数与默认值（`dim=`、`keepdim=`） | API 写法 | 写成多个重载名 |
| V0.6 | `@intrinsic` 声明 Tensor 运算符以及"结果类型由编译器计算"的可行性 | §3 | V1 先用普通 dunder 实现 |
| ~~V0.7~~ | ~~TypeSpec 携带 shapes~~ | 已决：形状没有语法，不进入 TypeSpec（D2） | — |
| V0.8 | 新增 `GC_KIND_STORAGE` 对 GC hooks、`std/gc` 统计和 JIT root 枚举的影响 | D5 | — |
| V0.9 | 方法名 `to[T]()` 与 `to(dev)` 共用一个名字（类型参数与值参数）是否可行 | API | 分别命名为 `to_dtype` 和 `to` |

**步骤**：

1. 在 `issues/` 中登记一个总 issue，叫"Tensor V0 语言能力 spike"，编号从 `issues.summary.md` 页首的下一个可用编号开始，把上表作为 checkbox 清单。
2. 每项写一个十几行的玩具 `.kl` 文件，用 `koala` 运行验证，结论写进该 issue 的详情页。
3. 有不可行项时，按退路修订本文对应章节，并在 §14 中记录。

**验收**：issue 中所有 checkbox 都有结论；本文据此修订完成。

---

### V1-min — 最小可用集（V1 的第一步，先于 V0）

**原则**：先做通最短的一条链路（GC Storage → Tensor 对象 → 广播内核 → `.kl` 声明 → ut → M0 demo），尽量避开 V0 中还没验证的语言能力。真碰到这些问题时，**逐个停下来讨论**，结论写回本文，不先集中做 V0。

**避开 V0 的方式**：

| V0 项 | V1-min 的做法 |
|---|---|
| V0.1 泛型默认参数 | 不用泛型：`Tensor` 就是非泛型类，**只支持 float32**；`Tensor[T]` 放到 V1 后续再做 |
| V0.2（已按 D12 定为方案 A） | V1-min 不提供比较方法和 bool dtype，V1 再加 |
| V0.4 变长、混合实参的 `__getitem__` | 不提供 `[]`；改用 `at(idx ...int)`、`narrow`、`select` |
| V0.5 关键字参数 | 只用位置参数加默认值（已有先例：`Buffer.__init__(capacity = 256)`） |
| V0.6 `@intrinsic`/编译期计算类型 | 运算符用普通 dunder，走现有 slot 路径；标量在左（`2.0 * t`）**不支持** |
| V0.7（已决，D2） | 形状没有语法；V1-min 不做编译期形状推导 |
| V0.9 `to[T]()` | 只有一种 dtype，不需要 |
| **Q-min-1（已决）** | Tensor 与标量混合运算用联合类型参数：`__add__(rhs Tensor[T] \| float64)`，`+=` 等同样处理，写法与 PyTorch 一致：`t * 2.0`、`t += 1.0`。语言已支持 union 成员里的类型参数（issue #80，2026-11-28）。标量在左（`2.0 * t`）需要反向运算符，放到 V2.1；整数字面量（`t + 2`）不会隐式转成浮点，要写 `t + 2.0` |
| 一元 `-` | 语义层只允许 int/float 使用一元 `-`，V1-min 用方法 `neg()`，不声明 `__neg__` |
| 创建函数 | 写成泛型 static 方法 `static func zeros[U](shape ...int) Tensor[U]`；static 方法不能使用类的 `T`。`U` 从目标类型推导（变量、赋值、参数、返回值），例如 `let w Tensor = Tensor.zeros(3, 4)`；没有目标类型时（如 `let w = ...` 或运算符的右操作数）要显式写：`Tensor.zeros[float32](3, 4)`，否则报 `cannot infer type parameter 'U'` |

**支持范围**：

| 类别 | 内容 |
|---|---|
| dtype | 只有 `float32`；标量参数用 `float64` 传入，内部转成 f32（相当于弱标量） |
| 维度 | 任意 rank（不超过 64），包括 0 维和大小为 0 的维；shape/strides 内联存前 6 维 |
| 创建（static） | `zeros(shape ...int)`、`ones(shape ...int)`、`full(v float64, shape ...int)`、`arange(start, stop, step = 1.0)`、`linspace(start, stop, num = 50)`、`from_list(data list[float64], shape ...int)`（扁平数据加 shape）、`scalar(v float64)`（0 维） |
| 属性 | `shape() list[int]`、`ndim()`、`numel()`、`stride() list[int]`、`is_contiguous()` |
| view | `reshape(shape ...int)`（支持 `-1`，能 view 时返回 view，否则拷贝）、`view(shape ...int)`、`transpose(d0 = -2, d1 = -1)`（默认交换最后两维，矩阵转置写 `transpose()`，不提供 `t()`）、`unsqueeze(dim = 0)`、`squeeze(dim)`、`narrow(dim, start, len)`、`select(dim, i)`、`expand(shape ...int)` |
| 拷贝 | `contiguous()`、`clone()`、`copy_(src)` |
| 运算 | `+ - * /`：右操作数是 Tensor（带广播）或 `float64` 标量，参数类型 `Tensor[T] \| float64`；`neg()`；ufunc：`exp log sqrt abs relu sigmoid tanh` |
| 归约 | `sum() mean() max() min()` 返回 0 维 Tensor；`sum_dim(dim = -1, keepdim = false)`、`mean_dim(dim = -1, keepdim = false)` |
| 线代 | `matmul(b)`：只支持 1D/2D（1D·1D、2D·1D、2D·2D），朴素 ikj 实现 |
| 原地 | `+= -= *= /=`（`__iadd__` 等），参数是 `Tensor[T] \| float64`；`fill_(v)`、`zero_()`、`copy_(src)`；写入 stride 为 0 的 view（expand 的结果）时 panic |
| 标量与输出 | `item() float64`、`at(idx ...int) float64`、`tolist() list[float64]`（扁平）、`fmt/to_str`、`allclose(b, rtol = 1e-5, atol = 1e-8)` |
| 内存 | `GC_KIND_STORAGE`、`gc_alloc_storage/gc_free_storage`、CPU 计数与阈值 `KOALA_STORAGE_MAX`、`SHARED` 标志、`t.free()`（走 `tensor_free`）、`gc.storage_used()` |
| 设备 | 冻结 `Device` 和 §6.2 的 ops 表，只实现 CPU；CPU 分配器先用 `aligned_alloc(64)`，不做缓存池 |
| 不做 | 其他 dtype、比较/bool/`where`、`[]` 索引和切片、`cat/stack/split/flip`、`dot/bmm`/batch 矩阵乘、随机数、`tensor_scope`、`device_scope`、内存重叠检测、version、READONLY、perm/wrap storage、`pending_storage`、缓存分配器、`DynTensor` |

由于不提供负 step 的切片，"禁止负 stride"自然成立。`narrow/select/transpose/expand` 都只会产生非负 stride。

**详细方案**：

- `include/runtime/gc.h`、`src/runtime/gc.c`：
  - 新增 `GC_KIND_STORAGE`；
  - `StorageObject *gc_alloc_storage(size_t nbytes, size_t align, int device)`：头部用 `gc_alloc_block(sizeof(StorageObject), GC_KIND_STORAGE, ...)`，数据区用 `backend->alloc`；
  - `gc_mark` 对 STORAGE 和 BYTES 一样处理：直接标黑；
  - `gc_free_block` 遇到 STORAGE 时先 `backend->free(data)`；
  - `gc_free_storage(s)` 显式释放；
  - `gc_storage_used/max`；
  - `GCStats` 增加 `storage_alive/storage_bytes/storage_freed`。
- `include/runtime/device.h`、`src/runtime/device_cpu.c`：`Device{kind, index, driver*}`，由 `device_make(kind, index)` 构造；ops 表按 §6.2 一次性定好（`DeviceOps`/`DeviceMemOps`/`DeviceCopyOps`/`DeviceSyncOps`/`DeviceExecOps`，由 `DeviceDriver` 持有），CPU 实现 `dev_ops/mem_ops/copy_ops`，`sync_ops` 空实现，`exec_ops` 只有 `launch`。
- `include/runtime/tensorobj.h`、`src/objects/tensorobj.c`：
  - `TensorObject`；
  - `_mark` 标记 storage；
  - `tensor_new_contig(shape, ndim)`；
  - `tensor_new_view(base, shape, strides, offset)`，会置上 `SHARED`；
  - `tensor_free`。
- `src/objects/tensor_kernels.c`：
  - 最小版 `TensorIter`：算广播形状，按 strides 做 N 维递归循环，最内层是 f32 循环；
  - 归约（全局归约和按维归约）；
  - matmul。
  - 这一版**不做**维度合并和重排，正确性优先；V1 后续再优化。
- `libs/std/builtin/tensor.kl`：`pub class Tensor` 加上 `@native` 声明；`src/modules/builtin.c` 的 `builtin_types[]` 中注册 `tensor_type`；`libs/std/gc` 增加 `storage_used()`。
- 测试：
  - C 级测试放在 `test/test-c`（GC storage）；
  - `.kl` 测试放在 `test/test-ut/test_tensor_*.kl`，每个 panic 分支配一个 `@should_panic`；
  - M0 demo 放在 `test/test-run`。
- 错误：形状不匹配、越界、维度非法、写入 stride 为 0 的 view，一律 panic。消息里带算子名和双方 shape，例如 `add: shapes [3, 4] and [5] are not broadcastable`。

**M0 demo（V1-min 的写法）**：

```koala
let x Tensor = Tensor.linspace(0.0, 10.0, 50)
let y = x * 2.0 + 1.0
var w = 0.0
var b = 0.0
for _ in range(2000) {
    let err = x * w + b - y
    w -= 0.01 * (err * x).mean().item()
    b -= 0.01 * err.mean().item()
}
# 断言 w≈2、b≈1
```

**步骤**（每步都要过 `./build-debug.sh && ./test-debug.sh`，JIT 和 `--no-jit` 都要跑）：

| 步骤 | 内容 | 验收 |
|---|---|---|
| M1 | GC：`GC_KIND_STORAGE` 及其接口、统计；`device.h` + CPU 后端 | `test-c`：分配、mark 保活、sweep 回收、显式 free、超阈值 panic、对齐 64 |
| M2 | `TensorObject`、注册类型、`tensor.kl` 骨架；创建函数、属性、`__str__`、`at/item/tolist`、`free()`、`gc.storage_used()` | ut：创建函数、属性、0 维、大小为 0 的维；`collect()` 后 `storage_used()==0` |
| M3 | view 系列 + `contiguous/clone/copy_`；`SHARED` | ut：修改 view 后原 Tensor 跟着变；对 view 调用 `free()` 不释放共享的 Storage；非法维度 panic |
| M4 | 广播迭代器 + `+ - * /`（Tensor 和标量，见 Q-min-1）+ `neg` + ufunc + 原地运算 | ut：广播的各种组合；不能广播时 panic；写入 expand 的 view 时 panic |
| M5 | 归约 + `matmul` + `allclose` | ut：全局归约和按维归约、keepdim、空 Tensor；matmul 结果与手算一致 |
| M6 | M0 demo + `bench/` 加入 tensor 基线 | demo 收敛；bench 数字写入本文 §14 |

---

### V1 — Eager CPU 核心（V1-min 之后的补全）

**支持范围**（以下是最终范围；V1-min 已经做过的部分，在 V1 中只扩展 dtype 和形态）：

| 类别 | 内容 |
|---|---|
| dtype | `bool`、`int32`、`int64`、`float32`、`float64`（`float16/bfloat16` 在 V2 加入，只做存储和转换） |
| 维度 | 任意 rank（最多 64），包括 0 维和大小为 0 的维 |
| 创建 | `from_list`（支持嵌套）、`zeros/ones/full/empty`、`*_like`、`arange`、`linspace`、`eye`、`scalar`、`rand/randn/randint`（共用 `std/random` 的种子） |
| 属性 | `shape`、`ndim`、`numel`、`dtype`、`device`、`stride()`、`storage_offset()`、`is_contiguous()`、`len()` |
| view | 基本索引（int/slice/newaxis/`...`）、`view`、`reshape`、`transpose`、`permute`、`unsqueeze`、`squeeze`、`expand`、`narrow`、`select`、`split/chunk`、`flatten`、`contiguous` |
| 拷贝 | `clone`、`copy_`、`to[T]()`、`flip`、`cat`、`stack` |
| 运算 | 按 §3 的规则做提升（V1 在**运行时**计算提升，V2 改到编译期）；`+ - * /`、`neg`、`pow`、比较方法 `eq/ne/lt/le/gt/ge`、`equal`、`& \| ~`（bool）、`where`、`masked_fill/masked_fill_`、`clamp` |
| ufunc | `exp log log2 sqrt rsqrt abs sign sin cos tanh sigmoid relu floor ceil round` |
| 归约 | `sum mean max min argmax argmin prod any all`，支持 `dim=int/tuple/nil` 和 `keepdim` |
| 线代 | `matmul`（1D/2D/batch 广播）、`dot`、`mm`、`bmm`；朴素的分块 ikj 实现，不依赖 BLAS |
| 原地 | `zero_ fill_ copy_ clamp_`、`+= -= *= /=`、`t[idx] = v`（基本索引） |
| 标量 | `.item()`、`at(...)`、`tolist()` |
| 工具 | `__str__`（带精度控制，大 Tensor 折叠显示）、`allclose/isclose/equal` |
| 内存 | `GC_KIND_STORAGE`、`gc_alloc_storage` 系列接口、`SHARED`、`tensor_free`、`free(t)`、`with tensor_scope()`、`gc.storage_used()` |
| 设备 | 冻结 `Device` 类型与 `DeviceBackend` vtable，只实现 CPU；`device_scope` |
| 规则 | 禁止负 stride；禁止写入重叠的 view；`READONLY`；内存重叠检测；version 加 1 |
| 不支持 | 编译器内置（标量在左的 `2.0 * t` 暂时不通，写成 `t * 2.0`）、高级索引、autograd、融合 |

**详细方案**：

- 新增文件：
  - `include/runtime/tensorobj.h`、`src/objects/tensorobj.c`：Tensor 对象、视图、迭代器；
  - `include/runtime/device.h`、`src/runtime/device_cpu.c`：vtable 与 CPU 分配器；
  - `src/objects/tensor_kernels.c`：pointwise、归约、matmul 的 C 内核；
  - `libs/std/builtin/tensor.kl`：`@native` 声明；
  - 包内 `test/`：单元测试。
- 修改文件：
  - `include/runtime/gc.h`、`src/runtime/gc.c`：STORAGE 块的分配、标记、sweep、统计；
  - `src/modules/gc.c`、`libs/std/gc/*.kl`：暴露 storage 统计；
  - `src/modules/builtin.c`：注册 `tensor_type`（照 `bytebuf_type` 的先例，在 `builtin_types[]` 中注册）；
  - `src/CMakeLists.txt`。
- 统一的广播迭代器 `TensorIter`（参考 ATen 的 TensorIterator）：
  1. 合并可以合并的维度；
  2. 按 stride 从小到大重排维度，让内层循环访问连续内存；
  3. 计算输出形状，分配输出；
  4. 最内层循环交给按 dtype 特化的内核函数指针。

  所有 pointwise、比较、`where` 和 `copy_` 都走这一个入口。
- 运行时类型提升：V1 用一张 `result_type(dtype_a, dtype_b, a_is_scalar, b_is_scalar)` 查找表实现。V2 编译期使用同一张表（放在 `include/runtime/tensor_dtype.h`，编译器和运行时共用），保证两边结论一致。
- 错误语义（最小化 panic 的原则）：
  - 形状、dtype、设备不匹配和越界属于程序错误，直接 panic，报错信息包含算子名，以及双方的 shape、dtype、device；
  - 不提供用返回值表示失败的 API。

**步骤**：

| 步骤 | 内容 | 验收 |
|---|---|---|
| V1.1 | GC：`GC_KIND_STORAGE` + `gc_alloc_storage/perm/wrap/free_storage` + 每个设备单独的计数和阈值 + `GCStats` 新增字段 + `pending_storage` 队列（CPU 上只是空的骨架） | C 级 ut（`test/test-c`）：分配、标记、回收、阈值 panic、wrap 时调用 deleter |
| V1.2 | `device.h`：`Device`、`DeviceBackend` vtable、CPU 缓存分配器、`empty_cache/memory_stats` | C 级 ut：对齐、按档位复用、mmap 大块分配 |
| V1.3 | `TensorObject` 结构，`_mark`/`_free`，`tensor_free`，`SHARED`；在 `tensor.kl` 中声明创建函数和属性；`__str__` | `.kl` ut：创建、属性、打印；运行后 `gc.collect()` 之后 `storage_used == 0` |
| V1.4 | view 系列操作 + 基本索引 + 负 stride 检查 + `contiguous/clone/copy_` + `READONLY` | ut 覆盖每个 view 函数（修改 view 会改到原 Tensor）；每个 panic 分支用 `@should_panic` |
| V1.5 | `TensorIter` + 广播 + 运行时类型提升 + 算术和比较运算 + `where/clamp` + ufunc | ut：对照 NumPy 的参考值（手写在测试里）；广播错误时 panic；提升规则表逐条测试 |
| V1.6 | 归约（dim/keepdim、整数提升、f16 用 f32 累加、NaN 传播）+ `argmax` | ut：对每个维度做归约；0 维和空 Tensor 边界 |
| V1.7 | 原地运算 + 内存重叠检测 + version + `t[idx] = v` | ut：`a += a.transpose()` panic；写 expand 出来的 view 时 panic |
| V1.8 | `matmul/dot/mm/bmm`（包括 batch 广播）、`cat/stack/split/flip`、`rand/randn` | ut：与朴素三重循环的结果对比 |
| V1.9 | `tensor_scope`、`free(t)`、`at/item/tolist/allclose`；M0 demo；bench 基线 | M0 demo 用 `t * 2.0` 的写法通过；`bench/` 新增 tensor 项，记录基线数字 |

---

### V2 — 编译器内置

**新增支持**：

| 类别 | 内容 |
|---|---|
| 类型检查 | `Tensor[T]` 被编译器识别（`type_is_tensor`）；结果 dtype 在编译期按 §3 算出（与运行时共用同一张表）；`2.0 * t` 这种标量在左的写法；`if t` 编译期报错；原地运算如果把结果降到更低的类别，编译期报错 |
| 形状推导 | 按 §2.2 自动推导（不加任何语法）；已知形状用于 L1 优化；推导出必然不匹配时给 warning；`--dump=tensor-shape` |
| `Array` 删除 | 删除 `array_type` 语法和 `ARRAY` 关键字 |
| dtype | 加入 `float16/bfloat16`（存储，计算时用 f32）、`int8/int16/uint8` |
| opcode | `OP_TENSOR_BINOP/UNOP/REDUCE/LOAD/STORE` |
| JIT | `OP_TENSOR_LOAD/STORE` 内联地址计算和 load/store；其余 opcode 走 helper |
| L1 优化 | §9.2 全部内容：逃逸分析加自动 free、原地化、标量访问折叠、常量 Tensor 写入 klc |
| 其他 | `DynTensor` 与 `as[T]()`；高级索引（整数 Tensor 和 bool 掩码的 `index/index_put_`） |

**详细方案**：

- 类型系统：
  - `include/parser/typespec.h` 新增 `type_is_tensor`，以及 `TypeSpec` 中的 shapes 字段（可选）；
  - `src/parser/koala.y` 恢复 `$$->unresolved.shapes`；
  - `src/parser/parser_expr.c` 的 `parse_binary` **只新增分支**：lhs 或 rhs 是 Tensor 时转到 `tensor_binary_type()`。
- IR 与后端：
  - `include/common/opcode_list.h`、`src/parser/irgen.c`（新增 `emit_tensor_*`，参照 `irgen.c:1030` 按类型特化的先例）；
  - `backend/isel.c`、`lower_operands.c`、`klr/printer.c`；
  - klc 的读写（`write_klc.c`/`read_klc.c`）支持常量 Tensor。
- VM/JIT：`include/runtime/vm_ops.h`、`src/jit/jit.c`；同步更新 `src/jit/DESIGN.md` 的 opcode 核对表。
- 逃逸分析：新增 `src/parser/opt/tensor_l1.c`，在 pass 管理器（`opt/pass.c`）中注册，默认开启，可用 `--no-tensor-l1` 关闭。

**步骤**：

| 步骤 | 内容 | 验收 |
|---|---|---|
| V2.1 | `type_is_tensor` + 编译期 dtype 提升（共用表）+ 标量在左 + 禁止把 Tensor 当条件 | `test-kl`：每种非法组合都有一个编译错误用例；`test-run`：编译期推出的类型与运行时 `dtype` 一致 |
| V2.2 | 形状推导分析（常量 Tensor、创建函数、Meta 传播、内联后传播）+ `--dump=tensor-shape` + 必然不匹配时给 warning；删除 `Array` 语法 | `test-ir`：dump 结果中各种情况的推导是否正确；`test-kl`：必然不匹配时只有 warning，可以编译通过、运行时 panic |
| V2.3 | 专用 opcode：irgen、isel、VM、printer、klc 一起改 | 全量回归；`--dump` 能看到 `OP_TENSOR_*` |
| V2.4 | JIT 中 `OP_TENSOR_LOAD/STORE` 内联 | JIT 与 `--no-jit` 下 ut 都通过；bench：`at()` 循环比 V1 快 |
| V2.5 | L1：逃逸分析与自动 `tensor_free`、原地化改写 | ut：循环 1 万次后 `storage_used` 保持平稳（不靠 `gc.collect()`）；差分测试证明原地化改写前后结果不变 |
| V2.6 | 常量 Tensor 写入 klc + perm storage + READONLY | ut：写常量 Tensor 时 panic；klc 往返（写出再读回）测试 |
| V2.7 | f16/bf16/int8 等 dtype、`DynTensor`、高级索引 | ut：转换往返；`np.eye(K)[y]` 场景 |

---

### V3 — 图编译 v1（推理和前向）

**新增支持**：

| 类别 | 内容 |
|---|---|
| 抓图 | 基本块内静态抓图；按 §9.3 的规则切分 graph break；`--dump=tensor-graph` |
| TIR | 纯数据流 DAG，节点包括 pointwise / reduction / matmul / view / const / input / output |
| pass | G1 规范化；G2 functionalize（受限版：区域内不能有 view 写入和原地修改，遇到就切断）；G3 按具体形状特化并加守卫（每张图最多 8 个变体） |
| 融合 | pointwise 链、reduction 前置（prologue）、多输出归约、matmul 后置 pointwise（epilogue） |
| 内存 | G5 arena 规划；输入死亡且没有 `SHARED` 时，输出复用它的内存 |
| 后端 | B1 分块解释执行 |
| 运行时 | `OP_TENSOR_GRAPH`：先检查守卫，不通过就退回 eager |
| 工具 | 差分测试框架；`KOALA_TENSOR_FUSE=0`、`KOALA_TENSOR_DETERMINISTIC=1` |

**详细方案**：

- 新增目录 `src/parser/tir/`：
  - `tir.c/h`：数据结构；
  - `capture.c`：从 KLR 抓图；
  - `canon.c`：规范化；
  - `shape.c`：调用 Meta 函数做形状推断；
  - `fuse.c`：融合；
  - `memplan.c`：内存规划；
  - `codegen_b1.c`：生成 B1 微操作序列；
  - `serialize.c`：序列化到 klc，带独立版本号，版本不匹配时退回 eager。
- 运行时新增 `src/runtime/tensor_graph.c`，包含守卫检查、变体缓存和 B1 解释器。
- V3 的 Meta 形状函数就是 V1 内核里的形状计算部分，抽出来作为独立函数，例如 `tensor_meta_add(...)`，供 eager 和 TIR 共用。
- 抓图 pass 是独立的新文件，不修改已有的 KLR pass。

**步骤**：

| 步骤 | 内容 | 验收 |
|---|---|---|
| V3.1 | 抽出 Meta 形状函数，eager 改为调用它 | 全量回归不变 |
| V3.2 | TIR 数据结构、打印器、序列化 | TIR 往返测试（序列化后读回一致） |
| V3.3 | 抓图 pass 与 graph break 报告 | `test-ir`：给出 graph break 原因的用例 |
| V3.4 | G1 规范化 + G3 形状推断和守卫 | `test-ir`：化简用例；守卫不通过时退回 eager 的用例 |
| V3.5 | 融合（pointwise、prologue、多输出归约、matmul epilogue） | M0 demo 只生成 1 个内核（通过 dump 断言） |
| V3.6 | arena 内存规划 + 原地复用输出 | 每次迭代的 storage 分配次数降为"只有输出"（通过统计断言） |
| V3.7 | B1 后端 + `OP_TENSOR_GRAPH` + 差分测试框架接入 `./test-debug.sh` | 全部 Tensor ut 分别在 eager 和 compiled 两种模式下运行，结果一致（归约在容限内）；bench 记录加速比 |

---

### V4 — 训练栈

**新增支持**：

| 类别 | 内容 |
|---|---|
| 分发 | 完整的 Dispatcher 和 DispatchKeySet（§7.2）；ATen 风格的 schema 解析；CompositeImplicit/Explicit |
| 自定义算子 | `@op` 与 `register_kernel/fake/autograd`；`opcheck` |
| view 和原地修改 | 完整的 functionalize（G2）；`CopySlices`；所有 view 都记录 `_base` |
| autograd | 叶子、grad_fn、SavedVariable 的 version 检查、`backward`、`autograd.grad`、`no_grad/enable_grad/inference_mode`、`autograd.Function`、hook、`gradcheck`、`detect_anomaly` |
| 训练库 | `koala/nn`（Module、Linear、Conv2d、ReLU/GELU、LayerNorm、BatchNorm、Dropout、Embedding、loss）、`koala/optim`（SGD、Adam、AdamW、lr scheduler）、`koala/data`（Dataset、DataLoader 单进程版） |
| 持久化 | `save/load`（`.npy`、safetensors 读写） |
| 不支持 | 高阶导数（V6）；训练图编译（V5） |

**详细方案**：

- 新增 `src/runtime/dispatch.c`：
  - 算子表以 `op × key` 为索引，每一项存内核指针；
  - 用 `kl_reg_op(lib, schema, key, kernel)` 注册，复用 NativeLib 的注册机制。
- 新增 `src/runtime/autograd.c`：
  - Node 和 SavedVariable 都是 GC 对象，要实现 `_mark`；
  - 引擎按依赖计数调度；
  - `retain_graph=false` 时清空 SavedVariable。
- 反向公式：每个原生算子在 `libs/std/autograd/derivatives.kl` 中登记反向规则（参照 PyTorch 的 `derivatives.yaml`）。规则用 Koala 写，本身也是 Tensor 运算，所以能被 V5 的 AOT 联合图复用。
- `nn/optim` 用纯 Koala 加 Tensor 实现。优化器的参数更新走 `torch.no_grad` 语义下的原地运算，多个参数的更新留到 V5 的 `foreach` 融合。

**步骤**：

| 步骤 | 内容 | 验收 |
|---|---|---|
| V4.1 | schema 解析 + Dispatcher + 把 V1 的内核迁移到注册表 | 全量回归不变；`--dump=dispatch` 能看到分发路径 |
| V4.2 | 完整的 functionalize + `_base`/CopySlices | 差分测试：在有 view 写入、原地修改的场景下，eager 与 compiled 结果一致 |
| V4.3 | autograd 核心：叶子、grad_fn、引擎、SavedVariable 的 version 检查、`no_grad`/`inference_mode` | ut：`gradcheck` 覆盖每个可微算子；原地修改了已保存的张量时 panic |
| V4.4 | `autograd.Function`、hook、`detect_anomaly` | ut |
| V4.5 | 自定义算子 `@op` 和 `opcheck` | 示例 `rms_norm`：eager、compiled、autograd 三条路径都通过 `opcheck` |
| V4.6 | `koala/nn`、`koala/optim`、`koala/data` | MLP 在合成数据上训练收敛；与手写梯度的结果一致 |
| V4.7 | `.npy` 与 safetensors 读写 | 往返测试；读取 PyTorch 导出的 safetensors（样例数据离线生成） |

---

### V5 — 性能与设备

**新增支持**：

| 类别 | 内容 |
|---|---|
| 训练图 | AOT 联合图（前向和反向一起融合）；保存还是重算（v1 规则）；`foreach` 优化器融合 |
| 后端 | B2 JIT SIMD（SSE2/AVX2，前提是 bench 证明有必要）；B3 C 代码生成并缓存 .so |
| CPU 性能 | 内核用线程池并行（内核内部没有 GC 点）；分块 matmul，可选接 BLAS（以独立后端库提供，核心不依赖它）；NHWC 和权重预打包 |
| 设备 | `std/cuda` 后端库：vtable、CUDA 缓存分配器、流和事件、`ready_event` 加 `pending_storage` 的异步释放、`to(cuda)`、`pin_memory`、`non_blocking`、`graph_compile`（可选） |
| 加载 | mmap 加载 safetensors 和 gguf（`gc_wrap_storage`，READONLY） |
| 互操作 | DLPack |

**步骤**：

| 步骤 | 内容 | 验收 |
|---|---|---|
| V5.1 | AOT 联合图 + 保存还是重算 | MLP 训练中，compiled 与 eager 的 loss 曲线在容限内一致，且更快 |
| V5.2 | 线程池内核 + matmul 分块优化 | bench：多核加速比 |
| V5.3 | B3 C 代码生成与缓存 | 差分测试通过；缓存失效测试 |
| V5.4 | B2 JIT SIMD（以 bench 数据为前提） | 相对 B1 的加速比 |
| V5.5 | mmap 加载、safetensors/gguf、DLPack | 加载大文件不拷贝数据（统计确认 storage 为 EXTERNAL） |
| V5.6 | `std/cuda` 后端（独立库，可选构建） | 同一份 MLP 代码 `.to(cuda(0))` 后训练通过；在没有 GPU 的环境下构建和测试自动跳过 |

---

### V6 — 高级特性

| 类别 | 内容 |
|---|---|
| 形状 | 编译器内部的符号维（推导出"这两个维度相等"之类的关系，不加语法）、按 batch 分桶、符号形状守卫 |
| 抓图 | `@compile`：跨基本块抓图，生成 `cond/while_loop` 节点 |
| autograd | 高阶导数 `create_graph=true`；`koala/func` 提供 `grad/vjp/jvp/vmap` |
| 精度 | autocast 作用域；fp8；complex64/128 |
| 量化 | `QTensor[Q]`（int4/int8/Q4_0 块量化），把反量化融合进 matmul（W4A16） |
| 设备 | NPU（昇腾）后端，以 `graph_compile` 为主；ROCm；Metal |
| 自动 GC | 启用 `gc_storage_pressure_hook`：超过阈值时在安全点自动触发 GC（此项依赖 VM 的自动 GC 方案，单独立项） |

V6 的每一项都单独立项（在 `issues/` 中各自登记），本文只列方向，到时再细化方案和步骤。

---

## 12. 风险

| 风险 | 版本 | 缓解 |
|---|---|---|
| 只能手动 GC，训练循环里的中间 Tensor 堆积 | V1–V2 | 主要靠 `tensor_scope`、显式 `free`、L1 自动 free（V2）和 arena（V3）；阈值报错的信息里提示用户该怎么做；自动 GC 在 V6 单独立项 |
| `SHARED` 判定偏保守，导致原地复用的机会变少 | V2 | 先保证正确；用 bench 统计命中率，再考虑改成更精确的方案 |
| view 与别名让融合出错 | V3–V4 | V3 遇到 view 写入就切断图；V4 先 functionalize 再融合；差分测试兜底 |
| 浮点归约的顺序变化让回归测试结果不稳定 | V3 | 归约结果按容限断言；`KOALA_TENSOR_DETERMINISTIC` |
| 新增 opcode 和 klc 格式变化带来兼容问题 | V2–V3 | 每一步把 irgen、isel、printer、klc、JIT 核对表一起改；TIR 序列化有独立版本号 |
| 守卫和变体缓存导致反复重编译 | V3 | 每张图最多 8 个变体，超过后固定走 eager 并打印诊断；V6 引入符号维 |
| `parse_binary` 改动与其他正在进行的工作冲突 | V2 | 开工前检查 `git status`；只新增分支 |
| 设备异步释放 | V5 | 从 V1 起就定下 `ready_event` 和 `pending_storage` 的语义；CPU 上的实现为空，不影响 V1 |
| 规模膨胀 | 全部 | 每个版本有明确的不支持清单；V6 每项单独立项 |

---

## 13. PyTorch / NumPy 迁移对照

| PyTorch / NumPy | Koala | 说明 |
|---|---|---|
| `torch.tensor(data)` / `np.array` | `Tensor.from_list(data)` / `Tensor[float64].from_list` | 不写 T 时默认 f32；np 门面默认 f64 |
| `t.dtype`（运行时确定） | `Tensor[T]`（静态），另有 `t.dtype` 可在运行时查询 | 结果 dtype 在编译期就能确定 |
| `t.to(torch.float16)` / `astype` | `t.to[float16]()` | dtype 不同时拷贝，相同时返回自身 |
| `t.to('cuda')` | `t.to(cuda(0))` | — |
| `t.item()` | `t.item()` | 显式同步点 |
| `t[i, j]`（结果是 0 维 Tensor） | `t[i, j]`（0 维 Tensor）或 `t.at(i, j)`（标量） | `at` 走快速路径 |
| `if t:` | 编译错误 | 改用 `.item()`、`.any()` 或 `.all()` |
| `a[::-1]` | 报错 | 改用 `flip(dim)` |
| `t.data` | 不提供 | 改用 `detach()` |
| `t.add_(u)` / `t.sub_(u)` / `t.mul_(u)` / `t.div_(u)` | `t += u` / `t -= u` / `t *= u` / `t /= u` | 原地四则运算只用运算符；`fill_`、`zero_`、`copy_` 等没有运算符的保留 `_` 方法 |
| `t.t()` / `t.T` / `t.mT` | `t.transpose()`（默认交换最后两维） | 不提供单字母简写，名字要能看出含义 |
| `torch.set_default_dtype` | 不提供 | 显式写 T |
| Tensor 作为 dict key 或放进 set | 按对象同一性处理 | 比较内容用 `equal()` 或 `allclose()` |
| `a == b`、`a < b`（逐元素） | `a.eq(b)`、`a.lt(b)` | 运算符形式编译报错（D12） |
| `x[x < 0] = 0` | `x.masked_fill_(x.lt(0.0), 0.0)` | — |
| `torch.where(x > 0, x, 0.1 * x)` | `where(x.gt(0.0), x, x * 0.1)` | — |
| `(pred == y).float().mean()` | `pred.eq(y).to[float32]().mean()` | — |
| `torch.equal(a, b)` | `a.equal(b)` | 返回 bool |
| `a @ b` | `a.matmul(b)` / `matmul(a, b)` | 不新增 `@` 运算符（`@` 已用于装饰器） |
| `a ** b` | `pow(a, b)` | 语言中没有 `**` |
| `a / b`（`__truediv__`） | `a / b`（`__div__`） | 整数相除结果为 f32 |
| `torch.no_grad()` | `with no_grad() { }` | 基于 Withable |
| `torch.library.custom_op` | `@op(schema)` | §7.4 |
| `torch.compile` | 默认在编译期抓图；V6 提供 `@compile` | — |
| `del t` / Python 引用计数 | `free(t)` / `tensor_scope` / `gc.collect()` | Storage 由 GC 管理 |

---

## 14. 变更记录

- 2026-11-27：定稿 v1。决策 D1–D11 经多轮讨论确定：dtype 静态、rank 可选、0 维即标量、按 PyTorch 的提升规则、Storage 由 GC 管理且不用 refcnt、禁止负 stride、设备 vtable 先冻结、view/算子注册/autograd 按 PyTorch、两层优化。版本路线为 V0–V6。当前状态：设计完成，**尚未实现**。
- 2026-11-27：调整路线：V0 不单独做，实现时遇到问题逐项讨论；新增 **V1-min**（§11），作为第一个实现目标，分 M1–M6 六步。
- 2026-11-27：D12 比较运算定为方案 A：逐元素比较使用 `eq/lt/...` 方法，不重载 `==`/`<`；V0.2 已决。修订 §1、§2.3、§3 P8、§3.1（新增）、§11 V0/V1-min/V1、§13。
- 2026-11-27：§2.2 更正：Koala 的 class 都是 final，没有子类型。形状细化改为**同一类型上的形状标注**：标注与非标注的写法是同一个类型，可以互相赋值；在边界上做编译期检查或插入运行时守卫。V2.2 同步修改。
- 2026-11-27：D2 再次修订（取代上一条）：rank 和 shape **不需要任何语法**，没有注解也没有 `expect_shape`，由编译器自动推导；推导出来就优化，推导不出来就不优化；常量 Tensor 一定能推导出来；不报编译期形状错误，只给 warning。D11 改为删除 `Array` 语法。修订 §1、§2.2、§9.2、§11（V0.7、V1-min、V2、V2.2、V6）。
- 2026-11-27：泛型函数和泛型 static 方法可以从可变参数 `...U` 推导类型参数，推导不出来时报 `cannot infer type parameter 'U' for function '...'`；类型冲突的报错改为显示可读类型名。
- 2026-11-27：`tensor.kl` 写入 V1-min 全部接口声明（全为 `@native`，C 实现尚未开始，运行时调用会触发 not_impl）。Q-min-1 已决：标量运算用 `*_scalar` 方法名；一元 `-` 改为 `neg()`；创建函数为 `static func xxx[U](...) Tensor[U]`，调用要写 `Tensor.zeros[float32](...)`。M0 demo 写法同步修改。
- 2026-11-27：删除 `t()`：单字母名字看不出含义，是 PyTorch 的坏设计；同理不提供 `T`/`mT`。矩阵转置统一写 `transpose(0, 1)`。修订 §5、§11 V1-min 与 V1、§13。
- 2026-11-27：可以合理给出默认值的参数都加上默认值：`transpose(d0 = -2, d1 = -1)`（默认交换最后两维，矩阵转置就是 `transpose()`）、`unsqueeze(dim = 0)`、`sum_dim/mean_dim(dim = -1, keepdim = false)`、`linspace(start, stop, num = 50)`；`arange` 的 `step = 1.0` 和 `allclose` 的 `rtol/atol` 原本就有。
- 2026-11-27：语言新增"从目标类型推导泛型参数"：实参推导不出来的类型参数，从变量、赋值、参数（含关键字参数）、字段、`return` 的类型推导；实参优先，显式类型参数不会被覆盖；没有目标类型或目标是 `any` 时报错。Tensor 创建函数因此可以写 `let w Tensor = Tensor.zeros(3, 4)`。手册 Inferring Type Arguments 一节同步更新。
- 2026-11-28：Q-min-1 改为联合类型参数：`+ - * /`、`add_ sub_ mul_ div_`、`+= -= *= /=` 的参数改成 `Tensor[T] | float64`，删掉 `add_scalar` 等 8 个方法；语言侧支持 union 成员里的类型参数（issue #80）。M0 demo 改回自然写法 `x * w + b - y`。标量在左（`2.0 * t`）仍不支持。
- 2026-11-28：删除 `add_ sub_ mul_ div_`：与 `+= -= *= /=` 重复，原地四则运算只用运算符；`_` 后缀只用于没有运算符的原地方法（`fill_ zero_ copy_`，V1 的 `clamp_ masked_fill_`）。修订 §4.3、§5、§11 V1-min 与 V1、§13。
