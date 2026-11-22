# Koala 机器学习基础建设方案（Tensor + Plot）

目标：让 Koala 能端到端完成吴恩达机器学习课程的编程作业。
路线（已定）：**先内置 Tensor 类型打底，绘图以 webview 为后端**。
本文档为设计方案，不含实现代码改动。

> **本轮修订（评审 + 起点收敛 + 内置决策）**
> - §12：对本方案的评审意见（类型层缺口、课程覆盖面、缺失 API、工程缺口、结构问题）。
> - §13：**Tensor 作为编译器内置类型**的决策依据，以及比吴恩达课程更小的起点 **M0**
>   （一维 float64 + 梯度下降拟合直线，分 T0–T3 四层）。
> - §14：待用户确认事项与已确认结论。
> - §4.1 / §4.7 / §7 / §8 / §9 / §10 已按"编译器内置"路线同步修订：
>   原"parser/编译器零改动"的结论作废，改为"语法/词法不动，类型检查与后端有意支持"。

---

## 1. 目标与范围

### 1.1 基准课程

以 **2022 版 Machine Learning Specialization（Python/NumPy 版）** 为主要基准，
老版 Coursera ML（Octave 版）作业作为扩展覆盖。
Deep Learning Specialization 依赖 TensorFlow/PyTorch，**明确排除**（见 §10）。

### 1.2 验收标准

不是"API 看起来像 NumPy"，而是**用课程原始数据端到端跑通真实作业**，
并对课程讲义给出的参考数值做断言（见 §8 里程碑）。

### 1.3 作业需求矩阵

| 作业 | 核心依赖 |
|---|---|
| 单变量线性回归（梯度下降、代价函数、可视化） | Tensor 创建/逐元素运算/归约、`loadtxt`、scatter/plot |
| 多变量线性回归 + 特征归一化 | `mean/std`（axis）、广播、contour（代价曲面） |
| 逻辑回归（sigmoid、决策边界、正则化） | 逐元素 `exp/log`、布尔掩码索引 `X[y==1]`、scatter |
| 神经网络前向传播（coffee roasting lab） | `matmul`、`.T`、`reshape`、按列切片 `X[:, 0]` |
| 老版：normal equation | `pinv`（M4） |
| 老版：PCA / 异常检测 / 推荐系统 | `svd/eig`、协方差、`imshow`（M4） |
| 老版：K-means | 广播距离计算、`argmin(axis)`、scatter |

归纳为四个能力域：**Tensor 数值计算、matplotlib 式绘图、数据加载、小规模线代**。

> **评审修订（§12.3）**：上表偏乐观。2022 版课程中相当一部分 graded assignment 依赖
> TensorFlow / sklearn，数据是 `.npy`；老版多个 ex 依赖 `.mat` 与 `fminunc/fmincg`。
> **v1 目标应显式收敛为"覆盖 numpy 路径的作业"**，逐作业覆盖矩阵见 §12.3。

---

## 2. 跨语言先例对比与设计取舍

| 参照 | 核心机制 | 采纳 | 不采纳 |
|---|---|---|---|
| **NumPy** | C 连续缓冲 + shape/strides；view 语义；右对齐广播；ufunc；模块函数 + ndarray 方法双入口 | 内存模型、广播规则、view 语义、API 命名（Python-parity 原则） | fancy indexing 全集、`order='F'`、masked array、结构化 dtype |
| **PyTorch** | Tensor 一等公民；dtype/device 显式；autograd tape；`view` vs `reshape` 区分 | Tensor 作为单一核心对象（Object-First）；dtype 显式标注 | autograd（v2 展望，见 §4.16）、device/GPU、`requires_grad` |
| **Julia** | 数组内建于语言；列主序；1-based；多重分发 | "数组是核心类型而非边缘库"的定位（支撑内置决策） | 列主序（与 NumPy 数据交换成本高）、1-based |
| **Go gonum** | `mat.Dense` 仅二维；BLAS 后端；绘图独立包 gonum/plot | 绘图独立包、数值核心 native 化 | 仅二维（课程需要 1D/2D 起步但不应锁死）、外部 BLAS 依赖 |
| **Rust ndarray / candle** | `ArrayBase` 泛型维度 + 所有权/视图分离；candle 的 Tensor+storage 分层 | storage 与视图分层、native 核心 + 语言薄壳 | 编译期维度泛型（Koala 泛型表达力不同，且课程规模不需要） |

**结论**：NumPy 的语义模型 + PyTorch 的"Tensor 一等公民"定位 +
Rust 式的 native 核心/语言薄壳分层。不引入任何外部数值库（BLAS/LAPACK），
课程矩阵规模小，朴素 C 内核 + 后续 SIMD 足够。

---

## 3. Koala 现状盘点（已逐项核实）

### 3.1 已具备

- **标量数学齐全**：`libs/koala/math/__math__.kl` 有 exp/log/sqrt/pow/isclose 等，
  sigmoid、代价函数的逐元素内核可直接复用。
- **运算符重载钩子**：dunder 机制（手册 §Operator Overloading），
  `list.kl` 已实现 `__add__/__iadd__/__getitem__/__setitem__/__getslice__/__setslice__`。
- **多维索引语法已支持（关键发现，零 parser 改动）**：
  - `src/parser/koala.y` 的 `index_expr_list` 产生式接受逗号分隔的 expr 与 slice_expr
    （`a[i, j]`、`a[:, 0]`、`a[1:3, ::2]` 均可解析）；
  - `src/parser/parser_expr.c:3158 parse_index_load` 把整个索引列表作为
    **多个调用实参**传给 `__getitem__`（`check_call_args(fn_sym->params, index->vec)`），
    store 侧同理走 `__setitem__`；
  - 语言支持变长参数（手册 §Tuples and Variadic Types，`func f(args ...)`），
    因此 `__getitem__(idx ...)` 可接收任意维数的 int/slice/Tensor 混合索引。
  - 注：以上核实于当前 redefined 分支工作区（parser 有用户的 switch WIP，
    本方案不触碰 parser 层）。
- **slice 是内置类型**：`libs/std/builtin/slice.kl`，切片表达式下降为 slice 值传参。
- **类型是一等对象**（commit 28bfb5ff）：`dtype=float32` 这类"类型作为值"的
  API 形态有语言支撑。
- **数据加载**：`koala/csv`（reader/writer/parser）、fs、io 齐全，`loadtxt` 只是薄封装。
- **native 类型先例**：`std/builtin/bytebuf.kl` 的 `Buffer`（native `__init__`/
  `__getitem__` 等）是 Tensor 的直接参照。
- **webview 已落地**：`libs/koala/webview/webview.kl` 的 `Window` 提供
  `set_title/set_size/navigate/set_html/init_js/eval/run/terminate/destroy`，
  系统 webkit2gtk，无捆绑浏览器 —— plot 的数据通道（`set_html` + `eval`）已够用。
- **itertools 完成**（12 组合子），zip/map 等课程辅助逻辑可纯 Koala 写。

### 3.2 缺口

- **全库无 ndarray/matrix/linalg**（grep 证实）。
- `koala/random` 无 `normal/gauss/randn`（权重初始化、数据生成需要）。
- 无绘图包（turtle 是教育向，非 ML 绘图）。
- 无矩阵乘运算符：lexer 中 `@` 仅作装饰器 token。**v1 用 `dot/matmul` 方法规避，不加语法**。
- JIT v1 仅覆盖 int/float/bool 算术，list 索引循环走 VM ——
  纯 Koala list-of-lists 写梯度下降内循环会慢，数值核心必须 native 化。

---

## 4. Tensor 类型设计（内置）

### 4.1 定位与命名

- **内置类型 `Tensor`（已确认：编译器内置）**：与 list/dict/Buffer 同层，
  壳在 `std/builtin/tensor.kl`，核心为 native C（与 Buffer 相同的组织方式）。
  **不止是"放在 builtin 包里的 native 类"**：类型检查器认识 Tensor、
  二元运算有专用下降路径与 opcode、JIT 有快速路径。决策依据与分层见 §13。
- 单一对象原则（Object-First）：全体系只有 `Tensor` 一个数值容器概念，
  不做 Vector/Matrix 平行类型；1D/2D/nD 只是 shape 不同。
- **Tensor 中心原则**：数值计算一律走 Tensor，`list[float]` 不作计算路径，
  只作数据边界（详见 §4.15）。
- numpy 风格的模块级函数放在 **`koala/np` 门面包**（§5），
  与 Tensor 方法操作同一对象，不构成旁路。

### 4.2 内存模型

```
Tensor = {
    storage:  native 连续缓冲（引用计数共享）
    dtype:    float64 | float32 | int64 | bool
    shape:    int[]（0-based 维数，v1 支持 1D/2D，结构上不限维数）
    strides:  以元素计的步长（支持 view）
    offset:   起始元素偏移
    base:     view 所引用的原 Tensor（保持 storage 存活）
}
```

- **行主序（C order）**，与 NumPy 默认一致；不支持 `order='F'`。
- **view 语义（NumPy 对齐）**：基本切片返回 view（共享 storage，改 view 即改原数据）；
  `reshape` 在连续时返回 view、否则拷贝（NumPy 同语义）；
  布尔掩码/fancy 索引返回拷贝（NumPy 同语义）。
  一开始就做 view：若 v1 先做拷贝语义，后续改 view 是破坏性变更。
- **storage 生命周期**：跟随现有对象生命周期模型（Buffer 先例），
  view 持有 base 引用保证缓冲不被提前释放；不依赖 tracing GC。
- **拷贝控制**：`copy()` 显式深拷贝；`astype(dtype)` 总是拷贝。

### 4.3 dtype 与提升规则

- v1 集合：`float64`（默认）、`float32`、`int64`、`bool`。
  dtype 参数用类型一等对象表达：`Tensor.zeros((2, 3), dtype=float32)`。
  **注意：dtype 是"静态类型参数 `Tensor[T]`"还是"运行时字段"尚未拍板，见 §13.2；
  该决策会改变本节与 §4.4 的书写形态（方案 A 下写作 `Tensor[float32].zeros((2,3))`，
  `dtype=` 由 `koala/np` 门面提供）。**
- 算术提升（保守、无静默精度丢失）：
  - `bool` 参与算术 → 提升为 `int64`；
  - `int64 + float*` → `float64`；`float32 + float64` → `float64`；
  - 同 dtype 保持同 dtype。
- 显式转换 `astype()`：float→int 的截断/饱和行为与运行时现行语义保持一致
  （现行：运行时 float→int 饱和），在手册中写明。
- 比较运算（`< > == !=` 等）产出 `bool` Tensor，供布尔掩码使用
  （**前提是语言允许 `__eq__` 返回非 bool，T0-a spike 验证，见 §13.4**）。
- **待补的语义细节（§12.4）**：int64 做 `/` 是否提升为 float64；int64 求和溢出；
  `log(0)` 的 NaN/inf 传播与 `max/min` 遇 NaN 的行为；
  `a += a.T` 这类输入输出内存重叠的原地运算如何处理。这些都要写进手册并用 ut 固化。

### 4.4 创建 API（numpy 同名同语义）

| API | 说明 |
|---|---|
| `Tensor.array(data, dtype=...)` | 从 list/嵌套 list 构造（课程数据入口） |
| `Tensor.zeros(shape, dtype=float64)` | shape 用 tuple |
| `Tensor.ones(shape, ...)` / `Tensor.full(shape, v, ...)` | |
| `Tensor.arange(start, stop, step)` | 半开区间，同 np.arange |
| `Tensor.linspace(start, stop, num)` | 含端点 |
| `Tensor.eye(n)` | 单位阵（M4 线代配套） |
| `np.loadtxt(path, delimiter=',')` | 复用 koala/csv，返回 float64 Tensor |
| `Tensor.rand/randn/...` | 见 §4.12 |

> **评审补充（§12.4）**：本表还缺 `zeros_like/ones_like`、`shape/ndim/size/dtype` 属性、
> `__str__` 打印、`allclose/isclose/array_equal`（§8 的参考值断言本身就要用）、
> `meshgrid`（contour 前置）、`np.save/np.load`（`.npy`）。这些在 M1/M2 补齐。

### 4.5 索引与切片（语法层零改动，机制已核实）

`__getitem__(idx ...)` / `__setitem__` 接收变长索引参数，每个参数可以是：

| 实参形态 | 语义（NumPy 对齐） |
|---|---|
| int（含负数） | 该维降一维；越界 panic |
| slice | 该维切 view；支持负端点与负 step |
| bool Tensor | 布尔掩码，返回拷贝（M2） |
| `a[i, j]` | 逗号索引 = 多实参，逐维处理 |
| `a[:, 0]` | 按列取，降维 |
| `a[1:3, ::2]` | 混合切片 view |

- 标量结果（全 int 索引）返回 dtype 对应的标量值（float/int/bool），不返回 0 维 Tensor
  （v1 不做 0 维 Tensor，规避一类边界复杂度；`a[i, j]` 课程场景全是标量取值）。
- **实现前置 spike（M1 第一件事）**：验证变长 `__getitem__` 在
  int/slice/Tensor 混合实参下的类型检查与 IR 下降路径可行（机制存在，组合未实测）。
- fancy indexing（整数数组索引）v1 不做，课程作业不需要。

### 4.6 广播（NumPy 右对齐规则）

1. 形状右对齐，缺维补 1；
2. 每维相等或其一为 1 才可广播；
3. 标量对任意形状广播；
4. 不兼容 → panic，报错信息给出双方 shape（教学友好）。

覆盖课程场景：`X - mean`（(m,n) 与 (n,)）、`X * w`（(m,n) 与 (m,1)）、
`J += reg * theta`（标量）等。

### 4.7 运算符（dunder 重载）

- 算术 dunder 用 **Koala 的真实名字**（已核实 `parser_expr.c:3791 get_binary_op_name`）：
  `__add__ __sub__ __mul__ __div__ __mod__ __neg__`。
  **注意：Koala 没有 `__truediv__`，除法钩子是 `__div__`；`__pow__` 不在运算符表内**，
  幂运算用 `pow(a, b)` 函数（§4.8），不做 `**` 语法。
- **标量在左（`2.0 * t`）现状不支持**：`parse_binary`（`parser_expr.c:4020`，
  第 4136 行起）只用 **左操作数** 类型查 dunder，且 `float64.__mul__` 只接受 `float64`；
  全库没有 `__radd__/__rmul__` 之类反向钩子。**库层无法自救**，
  必须由内置类型检查器新增"右操作数是 Tensor"的分支（§13 的 T1 层）。
- 比较：`__eq__ __ne__ __lt__ __le__ __gt__ __ge__`，产出 bool Tensor；
  `&&`/`||`（`__and__/__or__`）是 bool 专用、不可重载（`parser_expr.c:4145`），
  **布尔掩码的组合用位运算符 `&` `|` `~`**，对应 `__bitand__ __bitor__ __bitnot__`。
- 原地：`__iadd__ __isub__ __imul__ __idiv__` 等
  （梯度下降 `theta -= alpha * grad` 是课程高频写法）。
- **不加 `@` 矩阵乘运算符**：`a.dot(b)` / `np.matmul(a, b)` 覆盖全部课程需求；
  `@` 目前是装饰器 token，动 lexer 属于 parser 层（用户领域），收益低。

### 4.8 ufunc（逐元素，native 内核）

`exp log log2 log10 sqrt abs square sign maximum minimum clip floor ceil round`
+ 三角函数（复用 math 的 C 实现）。
标量版已在 `koala/math`，Tensor 版是同一内核的循环包装，避免两份实现。

### 4.9 归约

`sum mean std var max min argmax argmin prod`
- `axis=nil`（全局）、`axis=int`、`keepdims=bool`（默认 false，同 NumPy）；
- `std/var` 默认 ddof=0（同 NumPy）；
- `argmax/argmin` 返回 int64 Tensor（全局时返回标量 int）。
- 课程关键路径：特征归一化 `(X - X.mean(axis=0)) / X.std(axis=0)`。

### 4.10 形状操作

`reshape(shape)` `T`（属性，转置 view）`transpose(axes)` `flatten()`
`squeeze(axis=nil)` `expand_dims(axis)`
`np.concatenate(tensors, axis)` `np.vstack` `np.hstack` `np.stack(tensors, axis)`
- 课程场景：`X.reshape(-1, 1)`（-1 推断维）、`np.c_[...]` 用
  `np.column_stack` 替代（`c_` 是语法糖对象，不做）。

### 4.11 线代

- **M2**：`dot`（1D·1D 内积、2D·1D 矩阵向量、2D·2D 矩阵乘，同 np.dot）、
  `matmul`、`T`。native C 三重循环，ikj 序 + 连续访问优化；不引 BLAS。
- **M4**：`inv`（Gauss-Jordan）、`pinv`（经 SVD）、对称阵 `eig`（Jacobi）、
  小矩阵 `svd`（Golub-Kahan 或 Jacobi 单边）。课程矩阵 ≤ 几百维，朴素实现足够。
  奇异/不收敛 → panic 并给出原因。

### 4.12 随机数

`koala/random` 补齐（独立于 Tensor，先行小改动）：
- `normal(mu=0.0, sigma=1.0)`（Box-Muller 或 Ziggurat）、`randn` 别名语义对齐 np；
- Tensor 侧：`Tensor.randn(shape)`、`Tensor.rand(shape)`、`Tensor.randint(low, high, shape)`，
  共享 `random.seed()` 的全局状态（课程要求可复现）。

### 4.13 错误语义

- 语言无 try/catch（现状），所有形状/dtype/越界错误一律 **panic**，
  消息必须含操作名与双方 shape/dtype（课程调试体验关键）。
- 测试用 `@should_panic` 覆盖全部错误分支（随做随加，不批量补）。

### 4.14 性能策略

- 全部重内核 native C：广播循环、归约、matmul、ufunc。
  Koala 层只做参数解析与分发，内循环零 VM 开销。
- **不依赖 JIT 扩面**：Tensor 操作是 native 调用，JIT v1 现状即可。
- SIMD：v1 朴素 C（编译器自动向量化为主），手写 SIMD 留待基准证明需要后再做。
- 基准：bench/ 下加 Tensor 项（matmul、逐元素、归约），
  与纯 Koala list 循环对照，量化 native 化收益（预期 ≥ 一个数量级）。

### 4.15 与现有体系的边界（Tensor 中心原则）

- **Tensor 是唯一的数值计算载体**：一切数值计算（逐元素、归约、线代、
  广播、后续的 autograd/nn/CNN/RL）都发生在 Tensor 上。
  **绝不以 `list[float]` 作为计算路径**——不提供基于 list 的数值 helper、
  不做 list 的隐式数值化、np 门面函数只接收/返回 Tensor（或标量）。
- list 只出现在**边界**：数据输入（`Tensor.array(list)`）、结果输出
  （`tolist()`）、与通用序列生态（itertools/json/csv）的交接。
  进入计算前必须显式转 Tensor，计算完需要时才转回 list。
- plot 等外围包：**v1 spec/几何层先用 list**（渲染几何 ≠ ML 数值计算，且只在包内私有、
  不建公开 list-numeric helper）；Tensor 落地后再加 Tensor 入口（additive，几何层零返工）。
  ML 计算路径仍只走 Tensor，list 只在边界（输入/输出/interop）。
- v1 提供 `iter` 按首维迭代，够用即可；不做 Tensor 的完整序列协议。

### 4.16 autograd（v2 展望，v1 不做）

课程教的是**手写梯度**（梯度下降公式自己推），v1 不需要 autograd。
PyTorch 式 reverse-mode tape 列为 v2 候选（届时 Tensor 的 storage 分层可挂梯度字段）。
**原则修订（§12.6）**：由"不预留任何代码结构"改为"**不写未来代码，但不堵死**"——
例如 storage 保留 allocator/后端指针、dtype 用可扩展枚举，这类零成本的分层不视为过度设计。

---

## 5. `koala/np` 门面包

- 位置：`libs/koala/np`，纯 Koala 薄壳，无 native。
- 内容：numpy 同名自由函数（`np.zeros/np.array/np.dot/np.exp/np.loadtxt/...`），
  全部转发到 Tensor 方法/构造器；`np.pi/np.e` 转发 math 常量。
- 目的：课程代码几乎逐行可移植（`import numpy as np` → `import koala.np as np`
  的对应形态），同时不产生第二套数值对象。
- Tensor 方法与 np 函数是同一对象的两个入口（NumPy 本身即此形态），不违反 Object-First。

---

## 6. `koala/plot` 设计（webview 后端）

### 6.1 架构

```
Koala 侧                          webview 侧（系统 webkit2gtk）
─────────                         ─────────────────────────────
plt.plot/scatter/...              
  → Figure/Axes 对象树            
    （纯 Koala，v1 数据为 list      
      → tolist/JSON）             
plt.show()                        
  → 序列化为 figure spec JSON     
  → 注入 HTML 模板（内嵌渲染 JS）  
  → Window.set_html(html)         
  → Window.run()  ──────────────→ canvas 2D 渲染器（自研单文件 JS，
    （阻塞至窗口关闭）               vendored 进包，无外部 CDN 依赖）
```

- 数据通道只用已核实的 `set_html/eval/init_js`，**不需要 webview bind/dispatch**
  （该能力尚未落地，见 6.5）。
- JSON 序列化复用 `koala/json`；注意大数据量（imshow 图像）走
  `eval` 分块或 base64（复用 koala/base64），M3 实测决定。

### 6.2 对象模型与 API（matplotlib.pyplot 子集）

- `Figure` / `Axes` 两级对象；`plt.subplot(rows, cols, i)` / `plt.subplots(r, c)`。
- 绘图：`plot(x, y, label=, color=, linestyle=)`、
  `scatter(x, y, marker=, c=)`、`contour(X, Y, Z, levels)`、
  `imshow(Z, cmap=)`、`hist(x, bins)`。
- 装饰：`xlabel/ylabel/title/legend/xlim/ylim/grid/xticks/yticks`。
- `show()`：阻塞语义（= `Window.run()`，关窗返回），文档写明与
  matplotlib 非阻塞默认的差异；`close()` 对应 terminate/destroy。
- 输入 v1 接受 list（`list[float]` / `list[list[float]]`）；Tensor 落地后加 Tensor 入口
  （additive，几何层不改）。内部几何与序列化基于 list，不新增公开 list-numeric API
  （渲染几何 ≠ ML 计算，见 §4.15）。
- 坐标轴刻度/边距自动计算（nice ticks 算法），渲染质量以课程作业截图为对照标准。

### 6.3 渲染器（内嵌 JS，自研）

- canvas 2D：折线、散点、坐标轴、刻度、图例、子图网格。**渲染器是哑画师**：
  nice-ticks/边距/像素变换/图例与子图布局/marching squares 全在 Koala 侧解算，
  figure spec 携带已解算的像素几何（headless ut 可测；未来 LVGL/ui 后端廉价复用）。
- **contour：marching squares**（代价曲面作业硬需求），**Koala 侧实现**，
  levels 自动取 Z 的等分位或显式传入；spec 携带解算后的等值线像素几何，渲染器只哑画。
- **imshow：ImageData 像素填充** + colormap（v1：viridis 近似 + gray 两种）。
- 单文件、无构建链、无网络依赖；随包分发（vendored 注释注明自研与许可）。

### 6.4 课程验收场景

- 代价函数 J(θ0, θ1) 的 contour + 梯度下降轨迹叠加 plot；
- 逻辑回归决策边界：scatter（两类两色）+ plot（边界线）；
- 学习曲线 plot；老版 displayData → imshow 子图网格。

### 6.5 约束与后置项

- **交互（缩放/平移/点击回调）v1 不做**：依赖 webview bind/dispatch（未落地），
  v1 为静态图 + 关窗返回，课程作业全部够用。
- `savefig`：v1 后置；可行路线是 JS canvas→dataURL→base64→fs 写文件，M3 后评估。
- **`savehtml(path)` 建议提前到 M3（§12.5）**：本来就在生成 HTML，几乎零成本，
  且是无显示器环境下的唯一出口；配套后端开关 `KOALA_PLOT_BACKEND=html|webview`。
- **绘图测试策略（§12.5）**：`show()` 阻塞 GUI，无法进全量回归。
  断言只针对序列化后的 figure spec JSON；另需验证同一进程内多次 `show()` 是否可行。
- 嵌入式（LVGL 后端）绘图不在本方案范围。

### 6.6 matplotlib 兼容性：支持方式与不做清单（v1 收尾 2026-11-22）

**验证方法**：每个特性先以 venv 里的 matplotlib 3.11 实测默认值/错误消息/几何公式，
再按实测结果实现。以下各项中 mpl 完全一致的行为不再重复列出（margins 常量数据扩张
公式 `0.05*max(|v|,1)*(1+2m)`、`margin < -0.5` 报错、`tick_params` 全套校验消息、
`suptitle` 默认 `x=0.5, y=0.98, ha=center, va=top`、colorbar 几何 `(1-fraction-pad)`
收缩 + `pad` 默认 0.05 竖/0.15 横、contourf 刻度=levels、scatter 刻度=nice ticks、
`subplots_adjust()` 全空 no-op、`margins()` 无参返回当前值等，均已按实测对齐）。

**支持方式**：v1 快照"偏离清单"所列全部项现已落地支持，语义优先对齐 mpl 实测；
实现为简化近似（而非"不支持"）的地方在表中注明。

| 特性 | 支持方式 |
|---|---|
| marker | 全集：fmt 字符、int 0..11 tick/caret 表（mpl `_markers_int`）、全部命名 marker、`"None"/"none"/""/" "` 无 marker。`$...$` mathtext 画内文（简化：原样渲染 LaTeX 源码，不做数学排版）；自定义路径 `path:x0,y0;x1,y1;...`（简化 mpl 的 `Path` 对象为字符串，单位盒内闭合填充）。未知 spec panic 同 mpl 文案 |
| colormap | mpl 全注册表 91 个 base map + `_r` 反转后缀（`name_r(t) == name(1-t)`）；gray/grey 用解析公式（与 mpl 连续映射逐点相等；256 表在半整数处差一，故不用表）。未知名字 panic |
| scatter | `vmin`/`vmax` 显式归一化已支持；数值着色经 `cvals`（每点一值，走 `cmap` 默认 Normalize 同款映射）；`c`（常量色）与 `cvals` 互斥——静态类型无法重载 `c: str|list`，文档注释说明 |
| bar/barh | `bottom`/`left` 标量或逐条序列（同 mpl） |
| pie | `autopct` 格式串 + `textprops` 已支持；`wedgeprops` 只展开 `width`/`linewidth` 参数（dict 其余键不做，见不做清单） |
| errorbar/fill_between/stem/step | 参数全集：errorbar（yerr/xerr 标量或序列、非对称上下限序列、fmt/ecolor/elinewidth/capsize/errorevery/barsabove/六种 lims 开关）；fill_between（where/interpolate/step）；step（where=pre/post/mid）；stem（linefmt/markerfmt/basefmt/bottom/orientation） |
| boxplot | `whis` 标量/序列 + 6 个 props dict（box/whisker/cap/flier/median/mean，含 mpl 的 setdefault 与 flier `color` 惰性语义）；NaN/空数据集跳过（同 mpl 的 NaN 几何不可见）；位置刻度标签取整显示；`manage_ticks` 扩张语义同 mpl bxp |
| hist | `weights`/`density` 已支持（numpy 语义：density = count / bin宽 / 总权重；weights 长度不匹配 panic 同 ValueError "weights should have the same shape as x"；负权重合法、bar 向基线下方延伸；全零权重 → NaN 计数，bar 不画）；`bins` 仅 int。其余 kwargs 不做（见不做清单） |
| imshow | `extent`（覆盖既有极限，可反转）+ `aspect`（默认 `"equal"`，`"auto"`/正数/数字字符串；像素比 = (\|xspan\|/\|yspan\|)/aspect，按绘制时极限收缩、居中、不放大；非法值 panic 同 ValueError 文案）已支持。相同 extent 极限展开 ±0.5（mpl 展开量不同，视觉等价）；字符串 aspect 仅小写 `"inf"/"nan"` 特判（`"Inf"/"NaN"` 报转换错）。origin/interpolation/vmin/vmax/filternorm 不做 |
| tight_layout | 已支持（文本感知近似）：逐 axes 求装饰包围盒（tick 标签/轴标签/标题/图例/colorbar，文本宽度估计 ≈0.6×字号×len 而非实测字形）；mpl 语义对齐——`pad`/`w_pad`/`h_pad` 以字号为单位（×10pt/72inch）、`w_pad`/`h_pad` 是**相邻子图间**间隙（默认=pad，非外边距）、只有最外列/行的装饰参与外边距（mpl 按列/行分组）、suptitle 使上边距增加高度+一个 pad；layout 包围盒排除刻度线长度且 title/xlabel 仅贡献 y 范围、ylabel 仅贡献 x 范围（同 mpl `for_layout_only`）；空图 no-op（mpl 报 ValueError）、放不下 panic（mpl 警告后跳过） |
| tick_params | 尺寸单位为 **pt**（spec 输出按图 dpi 换算像素）；`labelsize` 支持相对字号字符串 `"xx-small"`~`"xx-large"`/`"larger"`/`"smaller"`（× 默认字号 10pt，同 mpl）；`which="minor"/"both"` 接受但忽略（无次刻度） |
| colorbar | fraction/pad 以**数据矩形**为基准（mpl 以含标签的 axes box）；`aspect` 宽度模型已支持（厚度=长度/aspect，默认 20，贴合数据矩形一侧；`aspect<=0` panic，mpl 不校验）；`extend` 三角箭头已画（`extendfrac` 默认 0.05，内部渐变按 `1/(1+lo+hi)` 收缩，三角用 under/over 端色 + 描边；contourf mappable 自身 `extend` 覆盖 colorbar 参数，线 contour 强制 neither，同 mpl）；`contour`（线）可映射（刻度=levels，vmin/vmax=首末 level）；bar 刻度外观固定默认（不随 axes `tick_params`）；第二次调用**替换**前一个 bar（mpl 可叠加）；省略 mappable 时取目标 axes 上**最后一个**可映射 artist（mpl 用全局 current-image 注册表）；`set_ticks` 越界值钳制到两端（同 mpl normalization），标签小数位用 `tick_decimals` |
| 刻度标签小数位 | 已对齐 mpl ScalarFormatter：步长 2→0 位、1.5/2.5→1 位（"0.0, 1.5, 3.0"）、0.25/0.75→2 位；frame 刻度与 colorbar 覆盖刻度一致 |
| savefig | `dpi` 已支持（nil="figure"，像素=figsize×dpi 线性；非正 panic 同 ValueError "dpi must be positive"）；`transparent` 已支持（PNG 跳过背景填充，图区/角点 alpha 0，同 mpl；JPG 无 alpha 忽略）；`bbox_inches="tight"` 已支持（Koala 侧遍历 spec 求 tight bbox，文本宽度估计 ≈0.6×字号×len 而非实测字形；`pad_inches` 默认 0.1 按图 dpi 换算；非法值 panic——mpl 3.11 此路径自身崩溃）；格式 png/jpg/jpeg（`format` 参数压过扩展名；不支持格式 panic 同 ValueError 文案但只列支持项；svg/pdf/eps 矢量不做）；需显示器（webview 抓 canvas），headless 用 `savehtml` |
| subplots | squeeze 语义已支持（`squeeze=true` 默认：1x1→Axes、1xN/Nx1→`list[Axes]`、MxN→`list[list[Axes]]`；`squeeze=false` 恒 2D；nrows/ncols 非正 panic 同 mpl 文案）。Koala 无联合返回类型，返回 `(Figure, any)`，调用端 `case is Axes/list[Axes]/list[list[Axes]]` narrowing；注意运行时 `is` 擦除 list 元素类型，须按已知 nrows/ncols 写单一匹配臂 |
| twinx/twiny | 已支持：同 cell 叠加第二个 axes、共享轴隐藏（twinx 隐藏 x 轴、twiny 隐藏 y 轴）、对侧画自己的轴、limit 双向联动、autoscale 取组内所有成员数据并集（mpl sharex/sharey view 语义，venv 实测对齐）；可链式多个 twin（twiny(twinx())） |
| show | 阻塞默认（可 `block=false` 轮询）；`close()` 对应 destroy。mpl 非阻塞默认——webview 需要事件循环驱动，文档化差异 |

**不做清单**（v1 明确排除）：3D `plot_surface`（§6.2 已定，用 contour 替代）；
交互缩放/平移/点击回调（依赖未落地的 webview bind/dispatch，见 §6.5）；
动画；LVGL 后端；多 colorbar 叠加；legend 拖动；savefig 矢量格式（svg/pdf/eps）；
hist 其余 kwargs（cumulative/bottom/histtype/align/orientation/rwidth/log/stacked、
`bins` 序列）；imshow 的 origin/interpolation/vmin/vmax/filternorm；
pie `wedgeprops` dict 全集（仅 `width`/`linewidth` 展开）。

---

## 7. 编译器 / 语言层影响评估（已核实）

> 本节原结论是"parser/编译器零改动"，**已按内置决策作废**。
> 现在的边界是：**词法与语法产生式不动；类型检查、IR/opcode、isel、VM/JIT 有意改动**。
> 下表的"现状"列均在当前工作区代码中核实过，文件:行号为核实时的位置，会随改动漂移。

| 项 | 现状（代码依据） | 结论 |
|---|---|---|
| 逗号多维索引 `a[i, j]` / `a[:, 0]` | `koala.y` 的 index_expr_list + `parse_index_load` 多实参下降 | 已支持，语法层零改动 |
| 变长 `__getitem__(idx ...)` | 语言有 `...` 变长参数 | 机制存在；int/slice/Tensor 混合实参的类型推断需 spike |
| **关键字参数 / 默认值** | `koala.y:3781/3788` 关键字实参产生式、`parser_expr.c: parse_keyword`、`Buffer.__init__(capacity = 256)` | **已支持**，`dtype=` / `axis=` / `label=` 写法可行（§12.2 的疑问已消除） |
| 联合类型 | `number.kl`：`__init__(x int64 \| uint64 \| float64 \| str)` | 已有 `A \| B` 形式，可用于索引、axis 参数 |
| **标量在左 `2.0 * t`** | `parser_expr.c:4020 parse_binary`，4136 行起只取 **lhs** 类型查 dunder；`float64.__mul__(rhs float64)`；全库无 `__radd__/__rmul__` | **不支持，且库层无法自救**；必须由内置检查器新增"rhs 为 Tensor"分支（§13 T1） |
| dunder 真实名字 | `parser_expr.c:3791 get_binary_op_name` | `__div__`（非 `__truediv__`）、`__bitand__/__bitor__/__bitnot__`；`__and__/__or__` 是 `&&`/`\|\|` 且仅 bool（4145 行）。§4.7 已同步 |
| 二元运算下降链 | irgen → `OP_BINARY_*`；`isel.c:121 find_binary_rule` 仅 int/uint/float/bool/optional 有专用规则，其余走 `isel_lower_num_ops`(323) → `OP_NUM_*` → `vm_ops.h:1647` → `kl_call_add` → `kl_call_slot(SLOT_ADD)` | 不做内置也能跑，但每次运算都动态查表分发；T2 用专用 opcode 消除 |
| dunder → slot 映射 | `common/klc.c:500 TPSLOT("__add__", SLOT_ADD)` | 新增 Tensor 运算符可沿用该映射 |
| `@intrinsic` | `parser.c:1461 is_allow_intrinsic_annotation`：**仅 `std/builtin`、`std/prelude` 可用**；`number.kl` 用了 200 处 | 放进 `std/builtin` 才能用 intrinsic —— 这是"内置"的硬理由之一 |
| irgen 按类型特化的先例 | `irgen.c:1030` 起：`type_is_str/range/tuple/list` → `emit_*_call` | 新增 `type_is_tensor` + `emit_tensor_*` 属于既有套路，不是另起机制 |
| 序列快速路径 | `opcode_list.h:1936/1965` 的 `OP_SEQ_GET/SET`（含 `_IMM`/`_SLICE`） | 可参照新增 `OP_TENSOR_GET/SET` |
| native 对象先例 | `objects/bytebuf.c:413 TypeObject bytebuf_type`（methdefs / `_free` / `gc_alloc_obj`）、`modules/builtin.c:274` 注册 | storage 生命周期与释放钩子照此实现 |
| JIT 现状 | `src/jit/DESIGN.md`：标量算术已内联 SSE2；`OP_NUM_*`（`jit.c:1117`）与 `OP_SEQ_GET`（`jit.c:2517`）走 helper | T2 可内联 Tensor 标量读写；融合内核属远期（T3/§11.7） |
| `@` 矩阵乘运算符 | lexer 中 `@` 是装饰器 token | 不加，`dot/matmul` 规避 |
| 泛型默认参数 / dtype 静态化 | **未核实** | **T0 之前的 spike 必须回答**，决定 §13.2 的 A/B 方案 |
| 新增 opcode 的连带影响 | klc 序列化（`write_klc.c`/`read_klc.c`）、`klr/printer.c`、JIT 的 opcode 全量核对（DESIGN.md §548） | T2 必须同步更新，见 §9 风险表 |

---

## 8. 里程碑与验收

测试策略：包内 `test/` 目录 `@test ut`（遵循 stdlib 组织规范），
用项目既定脚本 **`./build-debug.sh`（全量编译安装）+ `./test-debug.sh`（全量回归）**，
**JIT 与 --no-jit 双模式都必须过**；错误分支用 `@should_panic`，随实现随加测试。
（原文写的 `build-and-test.sh` 不存在，已更正。）

### M0 — 比课程更小的起点（一维 float64 + 拟合直线）

**先做 M0，再进入 M1。** 完整分层（T0–T3）、决策依据、受影响文件见 §13。
一句话：只用一维 float64 Tensor 跑通梯度下降拟合 `y = 2x + 1`，
不需要绘图、CSV、二维、广播、切片和线代，但已经打通
"native storage + 运算符 + 归约 + 编译器下降 + JIT/VM 双模式"这条主链路。

### M0.5 — 二维 / 广播 / view（M0 与 M1 之间的过渡）
二维 shape、右对齐广播、切片 view、带 axis 的归约、`dot/matmul/.T`。
- 验收：用矩阵形式 `X · theta` 重写 M0 的同一个 demo，结果与 M0 数值一致。

### M1 — Tensor 核心（补齐 M0/M0.5 之外的部分）
其余 dtype（float32/int64/bool）与提升规则、比较 dunder 与布尔 Tensor、
完整 ufunc 集、形状操作（reshape/transpose/squeeze/expand_dims/concat/stack）、
`__str__` 打印、`allclose`。
- 前置 spike：变长 `__getitem__` 混合实参的类型推断（右操作数分发已在 M0/T1 解决）。
- 验收：ut 全覆盖（含全部 panic 分支）；广播/切片/view 语义与 NumPy 对照用例；
  bench 基线建立。

### M2 — 线代基础 + 数据 + np 门面 → 第一个真实作业
dot/matmul/T 的完整形态、random.normal/randn、loadtxt、布尔掩码索引、`koala/np` 包，
以及 §12.3/§12.4 指出的必需补齐项（`.npy` 读写、`meshgrid`、`linalg.norm`、
一维整数数组索引、`random.permutation/shuffle/choice`）。
- **验收：移植单变量线性回归 lab**：断言课程参考值，再移植多变量 + 特征归一化 lab。
- **参考值版本必须先定**（§12.5）：原文的 J(θ)≈32.07、θ≈(-3.63, 1.17)、
  ex1data1.txt/ex1data2.txt 都是**老版 Octave 课程**的数值；
  2022 版 MLS 的 lab 用 w=2、b=1 初始化，参考值不同。二选一后再写断言。

### M3 — koala/plot → 可视化作业闭环
Figure/Axes、plot/scatter/contour/imshow/hist/subplot、show()。
- **验收：逻辑回归 lab 端到端**（sigmoid、布尔掩码分类、决策边界图、
  正则化代价断言）；代价曲面 contour 图与课程截图目视对照；
  神经网络前向传播 lab（coffee roasting，matmul + reshape + 按列切片）数值断言。

### M4 — 高级线代（老版课程覆盖）
inv/pinv/eig/svd、eye、column_stack。
- 验收：normal equation 与梯度下降结果互证；PCA（ex7）投影误差断言；
  异常检测（ex8）协方差 + 多元高斯密度断言。

### M5 — 收尾
K-means 完整 lab、学习曲线/验证曲线、手册 Tensor/np/plot 章节
（全部代码示例实跑）、docs 与 README 更新、bench 汇总。

每个里程碑结束编译 + 全量测试通过后停下汇报，不跨里程碑连做。

---

## 9. 风险与开放问题

| 风险 | 缓解 |
|---|---|
| **`parser_expr.c` 与用户的 switch WIP 冲突** | M0/T1 要改 `parse_binary`，与 switch WIP 同文件不同函数；开工前先确认工作区状态，改动范围只限新增分支，不动既有路径 |
| **新增 opcode 的连带面** | klc 序列化/反序列化、IR printer、isel/lower_operands、JIT opcode 全量核对（DESIGN.md §548）必须同批更新；T2 单独一层做，便于回滚 |
| **dtype 静态化 vs 运行时 dtype 未定** | §13.2 的 A/B 方案必须先拍板；这是**唯一会影响 API 兼容性的决策**，定错以后要破坏性返工 |
| **T3 融合的别名/逃逸正确性** | 只融合"中间结果无逃逸、无别名、未被原地修改"的纯表达式；不满足就不融合；用与未融合路径逐元素对照的 ut 固化 |
| 变长 `__getitem__` + slice/Tensor 混合实参的语义组合未实测 | M1 首个 spike，若受阻立即上报（语法层不擅动） |
| view 共享 storage 的生命周期错误（悬垂/泄漏） | 参照 Buffer 先例 + 专项 ut（view 存活期改写、base 释放时序）；项目有内存全释放的基线要求 |
| ~~标量右操作数分发机制不明~~ | **已查明**：无反向钩子，库层不可解，由 T1 的内置分支解决（§7、§13） |
| **课程作业依赖 TensorFlow/sklearn/.npy/.mat** | §12.3 的逐作业矩阵；v1 目标收敛为"numpy 路径作业"，其余明确排除或后置 |
| **随机数序列无法与 NumPy 对齐** | 自研 Box-Muller 不等于 MT19937；验收用例避开依赖 `np.random.seed` 的参考值，或另行实现兼容 RandomState（§12.5） |
| **绘图无法在 CI/无头环境验收** | `show()` 阻塞 GUI；改为对 figure spec JSON 做断言 + 提供 `savehtml(path)` 与后端开关（§12.5） |
| webview 大 JSON（imshow）性能 | M3 实测，必要时 base64/分块 |
| contour 渲染正确性 | marching squares 用已知解析曲面（如二次型）做像素级断言 |
| dtype 提升与现行标量语义不一致 | §4.3 规则显式写入手册，ut 固化 |

开放问题（待用户拍板，完整清单见 §14）：
1. **dtype 方案 A（泛型 `Tensor[T]`）还是 B（运行时 dtype）** —— 最高优先级，阻塞 T0。
2. **M0 的推进粒度**：只做 T0 后汇报，还是 T0–T2 连续推进、每层停一次。
3. 门面包包名：`koala/np`（推荐，短）还是 `koala/numpy`（形似）？
4. v1 是否需要 0 维 Tensor？与第 1 项联动（泛型化后返回类型统一，0 维的必要性下降）。
5. `savefig` 是否提前到 M3？本方案建议后置，但 `savehtml` 建议提前（几乎零成本）。
6. 课程基准版本：2022 版 MLS 还是老版 Octave（影响 M2 的参考值断言）。

---

## 10. 明确不做（v1 范围）

- TensorFlow/PyTorch/sklearn 对标；autograd（v2 再议）；GPU/device。
- 外部 BLAS/LAPACK 依赖；fancy indexing 全集；`order='F'`；结构化 dtype。
- `@` 运算符；**lexer / 语法（koala.y、koala.l）任何改动**。
  注意：原表述"parser/lexer 任何改动"已作废——按 §13 的内置决策，
  **类型检查（`parser_expr.c`）、IR/opcode、isel、VM/JIT 会有意改动**，
  只有词法与语法产生式不动。
- 绘图交互回调（等 webview bind/dispatch）、嵌入式绘图后端。
- masked array、np.c_ 语法糖对象。
- 0 维 Tensor：**改为待定**，取决于 §13.2 的 dtype 方案（泛型化后可能自然支持）。

---

## 11. 未来扩展：DL / CNN / RL（路线图，非 v1 承诺）

v1 的 Tensor 是全部后续能力的地基。本章给出从"课程作业"走向
"深度学习/卷积网络/强化学习"的演进路线。原则与 §4.16 一致：
**每一阶段独立立项、独立验收，v1 不写未来代码但不堵死扩展点**（措辞按 §12.6 修订）；
本章只锁定方向与依赖关系，具体设计到各阶段立项时再细化。

> 结构建议（§12.6，待确认）：本章篇幅约占全文三分之一，建议拆分到
> `docs/koala_ml_roadmap.md`，主文档只保留 §11.1 的依赖图与指引。
> 另：本章中"ggml 1.0.0 于 2026-11-13 发布"等时效性论断需注明来源与核实日期。

### 11.1 依赖总览

```
Tensor (v1, 本方案)
  ├─ autograd (v2 核心)
  │    └─ koala/nn + koala/optim + DataLoader (DL 训练栈)
  │         ├─ CNN：conv2d/pool 内核 + 图像数据
  │         ├─ RNN/Attention → Transformer 积木
  │         ├─ RL：Env 接口 + DQN/PPO（复用 DL 训练栈）
  │         └─ LLM 微调/训练（远期，见 §11.10）
  ├─ LLM 推理集成（独立支线，不依赖 autograd，见 §11.10）
  └─ GPU/NPU 加速（贯穿各阶段，见 §11.9）
加速演进：SIMD → 多线程 → GPU/NPU
```

### 11.2 autograd（v2 核心，参照 PyTorch）

- **动态图（define-by-run）+ reverse-mode tape**：与 Koala 命令式执行模型天然契合，
  不做静态图编译。
- Tensor 增加 `requires_grad` / `grad` / `backward()` / `no_grad` 语义；
  梯度挂在 Tensor 节点上而非 storage（view 共享 storage 但梯度语义独立，
  这正是 v1 storage/视图分层的收益）。
- 每个 v1 内核补 backward：ufunc 逐元素链式法则、matmul 转置乘、
  归约反向广播、**广播的反向是按轴归约求和**（v1 广播规则越严格，反向越简单）。
- 验收方式：用 M2/M3 已移植的课程 lab 做**手写梯度 vs autograd 数值互证**，
  课程作业天然成为 autograd 的回归测试集。

### 11.3 DL 训练栈（koala/nn、koala/optim）

- `nn`：Module/Parameter 基类、Linear、Sequential、ReLU/Sigmoid/Softmax/Tanh、
  MSELoss/CrossEntropyLoss（PyTorch 同名同语义，Python-parity 原则延续）。
- `optim`：SGD（momentum）、Adam；`zero_grad()/step()` 协议。
- DataLoader：batch 切分（v1 切片 view 直接可用）、shuffle（random 已有）、
  迭代器协议对接 itertools。
- 模型持久化：save/load，候选格式为 safetensors 式（header JSON + 连续缓冲），
  复用 koala/binary 与 koala/json；不兼容 pickle 类任意对象序列化。
- 验收：coffee roasting lab 用 nn 重写并与 v1 手写版互证；
  进而 MLP 跑通 MNIST 规模数据集。

### 11.4 CNN

- 算子：`conv2d`（stride/padding/dilation）、`max_pool2d`、NCHW 布局；
  backward 为 col2im + 转置卷积核。
- 实现路线：**im2col + matmul**（直接复用 v1/M2 的 matmul 内核，正确性优先）
  → 性能不足时再上直接卷积 + SIMD。
- 图像数据入口：PNG/JPEG 解码候选 vendored stb_image 单头文件
  （遵循 vendored 溯源规范：保留上游许可与下载地址注释）；
  可视化复用 v1 的 imshow。
- 性能前提：此阶段 SIMD 从"可选"变为"必须"（见 11.7）。
- 验收：小型 CNN 在 MNIST/Fashion-MNIST 上达到公开参考精度区间。

### 11.5 RL

- **Env 接口（gym 风格，Koala trait 表达）**：
  `reset() -> obs`、`step(action) -> (obs, reward, done, info)`、
  observation/action space 描述。环境实现可以是纯 Koala（CartPole 等经典控制）。
- 算法路线：DQN 先行（replay buffer 直接用 collections.deque，
  target network 就是 Tensor 拷贝 + nn 模块），再到 REINFORCE/PPO。
- 数值上**不需要新内核**，完全复用 DL 训练栈；新增的是环境抽象与训练循环模式。
- 环境渲染复用 plot/webview（静态帧序列起步，交互等 webview bind/dispatch 落地）。
- 验收：CartPole DQN 收敛曲线（reward 滑动均值达到公开参考水平）。

### 11.6 其他方向

- **RNN/LSTM/Attention**：matmul + softmax + 逐元素内核已覆盖绝大部分计算，
  主要是循环结构与 mask 语义；Transformer 积木在此之上组装。
- **数据集生态**：koala/vision 式数据集包（下载/缓存/标准划分），立项时再定。
- **低精度 dtype**：float16/bfloat16 沿 v1 dtype 枚举扩展点加入，
  服务于推理与混合精度训练（远期）。

### 11.7 加速演进（贯穿各阶段）

1. **SIMD**：v1 靠编译器自动向量化；CNN 阶段手写 SIMD 内核成为必须。
2. **多线程**：内核级并行（matmul 分块、batch 并行）。线程模型属语言/运行时层，
   立项时需与用户另行讨论，不在本方案内预设。
3. **GPU/NPU**：见 §11.9 专节（库集成方案）。v1 不做 device 抽象
   （避免为假想需求设计），立项时以 storage 后端插件化方式落地。
4. **JIT 融合内核**：逐元素链的 kernel fusion 依赖 JIT v2 能力，远期评估。

### 11.8 推进原则

- 顺序锁定：**autograd → nn/optim → CNN → RL**，每步以前一步的验收达成为前提；
  LLM 推理集成是独立支线，不依赖 autograd，可在 Tensor 稳定后随时立项。
- 每阶段继续"真实任务驱动验收"（课程 lab → 公开数据集基准），不做 API 空转。
- 与用户领域的边界不变：语言层（线程模型、JIT、parser）改动一律先报告后立项。

### 11.9 GPU/NPU 方案与库集成

#### 11.9.1 架构前提

- **device 抽象落在 storage 层**：Tensor 增加 `device` 标识（cpu/gpu0/npu0…），
  storage 后端插件化（alloc/free/h2d/d2h/d2d + 内核分发）。
  视图/广播/shape 逻辑与设备无关，只有内核与缓冲驻留随设备变化。
- **显式迁移**（PyTorch 语义）：`a.to(device)` / `a.cpu()`，不做隐式跨设备运算；
  跨设备二元运算 panic 并提示。
- **dtype 现实**：消费级 GPU 的 float64 吞吐极低，设备侧以 float32/float16 为主
  （float16/bfloat16 沿 §11.6 的 dtype 扩展点加入）；float64 保留在 CPU。
- v1 不实现以上任何结构；本节锁定选型方向，接口细节立项时定。

#### 11.9.2 GPU 候选库对比

| 路线 | 库 | 许可 | 集成方式 | 评估 |
|---|---|---|---|---|
| CUDA | cuBLAS / cuDNN / cuRAND + 自写 kernel | NVIDIA 专有 EULA | 系统安装（nvcc 工具链，用户执行安装命令） | 训练最成熟；仅 NVIDIA；不可 vendored |
| ROCm/HIP | rocBLAS / MIOpen | 开源（MIT 系） | 系统安装 | 仅 AMD + Linux；API 与 CUDA 近似，可作第二后端 |
| Vulkan compute | 原生 Vulkan + SPIR-V，或 Kompute 框架 | Apache-2.0（Kompute） | vendored 或系统 | 跨厂商（含移动端）；样板代码多，训练生态弱 |
| OpenCL | CLBlast | Apache-2.0 | vendored 可行 | 跨厂商；生态萎缩，作备选 |
| SYCL/oneAPI | oneMKL / oneDNN | Apache-2.0 | 系统安装（Intel 工具链） | Intel GPU/CPU/NPU 统一入口，厂商绑定 |
| WebGPU | Dawn | BSD-3 | vendored 可行但体量大 | 面向未来的统一 API，现阶段过重 |
| **ggml** | ggml（llama.cpp/whisper.cpp 底层张量引擎） | **MIT** | **vendored 源码构建可行** | 现成多后端：CPU/CUDA/Metal/Vulkan/SYCL/OpenCL/HIP/OpenVINO/Hexagon；1.0.0 已于 2026-11-13 发布，API 趋于稳定 |

#### 11.9.3 两条实现路线

- **路线 A：自研内核 + device 后端接口**（PyTorch 式）。
  每个 op 写设备内核 + backward，控制力完整，与自研 autograd 无缝；
  工作量大（每算子 × 每后端），首选 CUDA 单后端起步。
- **路线 B：ggml 后端集成**。Tensor storage 映射到 ggml tensor，
  计算图调度/多后端分发交给 ggml；起步快、一份代码覆盖全部后端，
  且直接服务 LLM 支线（§11.10）。约束：受 ggml 算子集与图模型限制，
  其训练（backward）能力弱于推理，自定义算子需进 ggml 扩展。
- **建议**：推理优先的场景（含 LLM）走 B；自研训练栈走 A；
  两者可共存（B 做推理引擎，A 做训练内核），立项时按实际硬件拍板。

#### 11.9.4 NPU 方案

NPU 生态高度碎片化（Qualcomm Hexagon、Intel NPU、AMD Ryzen AI、
ARM Ethos-U、Rockchip/华为等各自 SDK），**不做逐厂商直连**，分三层：

1. **可移植推理路线（首选）：ONNX Runtime（MIT，C API）+ Execution Provider**。
   Koala 模型导出 ONNX（要求 nn 算子集标准化，立项时定义导出器），
   由 EP 落到具体硬件：QNN（Qualcomm）、OpenVINO（Intel NPU）、
   VitisAI（AMD）、TensorRT（NVIDIA 推理）、DirectML（Windows GPU 通吃）。
   集成方式：系统安装或官方预编译库链接（不可 vendored 二进制，遵循溯源规范）。
2. **嵌入式 NPU（呼应 FreeRTOS 下限目标）**：TFLite Micro + CMSIS-NN
   （均 Apache-2.0，可 vendored），面向 Ethos-U 类 MCU NPU；
   模型同样经导出路径离线转换。
3. **新兴直连路线**：ggml 的 Hexagon 后端（2026 年持续加强）
   提供 Qualcomm NPU 的免 ONNX 通道，随路线 B 评估。

**训练一律留在 CPU/GPU，NPU 只做推理**（各厂商训练栈均不开放或不成熟）。

#### 11.9.5 集成约束与开放问题

- 网络受限环境：专有工具链（CUDA/QNN/oneAPI）由用户执行安装命令，
  开源库优先 vendored（保留上游 LICENSE + 下载 URL 注释）。
- **开放问题：目标机器实际有什么 GPU/NPU？** 决定 G1 首个后端
  （有 NVIDIA → CUDA 或 ggml-cuda；无独显 → ggml-vulkan/CPU SIMD 先行）。

### 11.10 LLM 支持方向

#### 11.10.1 定位：两条支线

- **推理集成（近期可行，独立支线）**：绑定现成推理引擎跑本地 GGUF/ONNX 模型。
  **不依赖 autograd/训练栈**，只依赖 Tensor 稳定 + native 绑定机制，
  可在 M2 之后随时立项。
- **原生训练/微调（远期）**：LoRA 级微调依赖 autograd + nn + bf16 + GPU；
  从零预训练明确不在路线内。

#### 11.10.2 既有地基（已核实存在）

- `koala/openai`：远程 LLM API 客户端（本地引擎落地后可做 loopback 对照）。
- `koala/http` + `koala/sse`：足以对外提供 OpenAI 兼容的流式推理端点。
- `koala/json`、`koala/sqlite3`、`koala/random`（采样）、Tensor（logits 运算）。

#### 11.10.3 推理引擎选型

| 候选 | 许可 | 评估 |
|---|---|---|
| **llama.cpp C API（首选）** | MIT | GGUF 生态事实标准；tokenizer/KV cache/量化/采样内置；vendored 源码构建可行；与 §11.9 路线 B 同底座（ggml） |
| ONNX Runtime GenAI | MIT | C API，直接复用 §11.9.4 的 NPU EP 通道；模型需 ONNX 导出，生态小于 GGUF |
| ggml 直连自搭 transformer | MIT | 控制力最强但工作量最大，仅在需要深度定制时考虑 |

#### 11.10.4 需要补齐的组件

- **Koala 绑定层**：`koala/llm` 包，native 绑定 llama.h；
  API 形态参照 llama-cpp-python / transformers（Python-parity）：
  `Llama(model_path, n_gpu_layers=...)`、`generate(prompt, ...)`、
  流式输出对接 Koala 迭代器协议。
- **采样器**：temperature/top-k/top-p/min-p，logits 运算走 Tensor，
  随机源共享 `random.seed()`（可复现）。
- **chat template**：GGUF 内置模板的渲染（简单字符串模板引擎，不引 Jinja）。
- **embeddings**：`embed(text)` 返回 Tensor，供 RAG/相似度。
- **服务化**：koala/http 暴露 `/v1/chat/completions`（SSE 流式），
  用 koala/openai 客户端 loopback 自测 —— 本地模型 + 自家客户端闭环。
- **RAG 最小件**：embeddings + 向量检索；课程/小规模用 Tensor 暴力余弦即可，
  规模化再评估 sqlite-vec 扩展（许可与 vendored 方式立项时核实）。

#### 11.10.5 硬件与量化现实

- 消费级设备跑 0.5B–8B 量化模型（Q4_K_M 等）为主力场景；
  显存不足时 ggml 的 CPU/GPU 分层卸载（n_gpu_layers）兜底。
- NPU 通道（QNN/Hexagon）随 §11.9.4 成熟度跟进，不阻塞首发。

#### 11.10.6 验收

- 本地跑通一个小量化模型（如 Qwen2.5-0.5B-Instruct 级）：
  greedy 输出与 llama.cpp 参考实现逐 token 对照；
  流式 chat demo + OpenAI 兼容端点 loopback 测试；
  embeddings 余弦相似度与参考值断言。

---

## 12. 设计评审意见（本轮，含已核实/未核实标注）

本章是对 §1–§11 的评审结论。**标注约定**：
"已核实"= 在当前工作区代码中查过；"未核实"= 依据课程/生态的通用认知，立项前需再确认。

### 12.1 方向正确的部分（保留，不改）

- **真实作业驱动验收**（§1.2、§8）：不是"API 像 NumPy"就算完成。全文最有价值的一条原则。
- **view 语义第一天就做**（§4.2）：理由成立——先做拷贝语义、后改 view 是破坏性变更。
- **核心 native 化、不引 BLAS、不动语法**：课程矩阵规模下取舍合理。
- **Tensor 单一数值对象 + `koala/np` 薄门面**（§4.15、§5）：不会长出第二套数值体系。
- **plot 只用已落地的 `set_html/eval` 通道**，不依赖未落地的 bind/dispatch。

### 12.2 类型层缺口（P0，M0 之前必须解决）

原文 §7 只评估了 parser，没有评估类型检查器。逐项结论：

| # | 问题 | 状态 | 处理 |
|---|---|---|---|
| 1 | **同一函数按运行时参数返回不同类型**：`a[i,j]` 返回标量而 `a[:,0]` 返回 Tensor；`sum()` 与 `sum(axis=0)` 同理 | 风险成立 | 由 §13.2 的 **方案 A（泛型 `Tensor[T]`）** 根治：元素类型静态已知，`a[i]`/`sum()` 返回 `T` |
| 2 | **`__eq__` 返回 bool Tensor** 是否被语言允许；`if a == b` 的真值语义；dict/set 是否依赖 `__eq__` | **未核实** | T0-a spike 必答；NumPy 对 `if tensor:` 报错，Koala 需明确同样规则并写进手册 |
| 3 | 关键字参数与默认值（`dtype=`/`axis=`/`keepdims=`） | **已核实：支持**（`koala.y:3781/3788`、`parse_keyword`、`Buffer.__init__(capacity = 256)`） | 疑问消除，Python 形态可保留 |
| 4 | `axis` 可为 nil / int / tuple | 部分已核实：语言有联合类型 `A \| B`（`number.kl`）与 optional | 用 `int \| (int,int) \| nil` 表达；tuple 形式的 axis 可后置 |
| 5 | `dtype=float32` 的取值来自哪里（是否真有 `float32`/`int64` 类型对象） | 已核实类型是一等对象；具体名字需在 T0-a 确认 | 方案 A 下写成 `Tensor[float32]`，`dtype=` 由 np 门面转换 |
| 6 | **标量在左 `2.0 * t`** | **已核实：不支持**（无反向钩子，见 §7） | 必须由编译器内置分支解决 → §13 T1 |
| 7 | dunder 名字与 Python 不同（`__div__` 而非 `__truediv__`；无 `__pow__`；掩码用 `& \| ~`） | **已核实** | §4.7 已修订 |

### 12.3 课程覆盖面：需求矩阵偏乐观（P0，重写 §1.3）

§1.3 应改为**逐作业列出依赖并标注 v1 能否覆盖**。下表为依据课程结构的判断，**未逐份核实作业源码**，
立项时应以实际 lab 文件为准。

| 课程/作业 | 依赖 | v1 覆盖 |
|---|---|---|
| C1 线性回归（代价、梯度下降、可视化） | numpy + matplotlib | ✅ |
| C1 逻辑回归（sigmoid、决策边界、正则化） | numpy + matplotlib | ✅ |
| C2 W1 神经网络（coffee roasting） | numpy 版可做；graded assignment 用 TensorFlow，数据 `X.npy` | ⚠️ 仅 numpy 部分，且需 `.npy` |
| C2 W2 多分类 / softmax | TensorFlow | ❌（需 autograd/nn） |
| C2 W3 模型评估与选择 | sklearn（train_test_split / PolynomialFeatures / StandardScaler） | ⚠️ 需少量 helper |
| C3 K-means / 图像压缩 | numpy + `.npy` + PNG 解码 | ⚠️ 需 `.npy` 与图像解码 |
| C3 异常检测 | numpy（多元高斯需 `det`） | ⚠️ M4 需补 `det` |
| C3 协同过滤推荐 | TensorFlow GradientTape | ❌（需 autograd） |
| C3 强化学习 | TensorFlow + gym | ❌（§11.5） |
| 老版 ex2/ex4/ex5 | `fminunc`/`fmincg` 优化器 | ❌ 需 `optimize.minimize`（CG/BFGS） |
| 老版 ex3/ex4/ex7/ex8 数据 | `.mat`（Octave/HDF5） | ❌ 需 `.mat` 读取或离线转 npy/csv |
| 老版 ex6 | SVM | ❌ 明确排除 |

**结论**：v1 目标应显式收敛为"**覆盖 numpy 路径的作业**"，
并把 `.npy` 读写、优化器、少量 sklearn 风格 helper 列入里程碑或明确排除。
`.mat` 建议走"离线一次性转成 `.npy`/csv"的路线，不在语言里实现 Octave 格式解析。

### 12.4 缺失的 API（按课程使用频率排序）

**数值 / np（P1）**

- **打印**：`__str__`/repr，含精度控制与大数组折叠。课程到处在打印数组，原文完全没提。
- **断言工具**：`allclose` / `isclose` / `array_equal` + 明确的 rtol/atol 策略。
  §8 的"断言参考值"本身就要用它。
- **基础属性**：`shape`、`ndim`、`size`、`dtype`、`len()`、`zeros_like`/`ones_like`。
- **`meshgrid`**：contour 的前置，没有它 §6.4 的代价曲面画不出来。
- **布尔掩码配套**：`& | ~`（`__bitand__/__bitor__/__bitnot__`）、`where`、`any/all`、
  `isnan/isinf`、带掩码的 `__setitem__`（`a[a < 0] = 0`）。
- **线代补充**：`linalg.norm`（K-means、梯度检验）、`det`（ex8）、`outer`、`diag`。
- **排序与常用工具**：`sort/argsort`（推荐系统 top-k）、`unique`、`cumsum`、`tile/repeat`。
- **随机补充**：`permutation/shuffle/choice`。
- **最小整数数组索引**：只支持"一维、首轴"的 `a[idx]`。
  老版 one-hot 写法 `np.eye(K)[y]` 很常见，不必做 fancy indexing 全集。
- **`newaxis`**：`x[:, np.newaxis]` 在 lab 中高频。
- **持久化**：`np.save`/`np.load`（`.npy` 格式简单，建议进 M2）、`savetxt`。

**语义细节（写进 §4.3）**

- int64 做 `/` 是否提升为 float64；int64 求和的溢出行为。
- `log(0)` 产生的 NaN/inf 如何传播；`max/min` 遇 NaN 的行为（是否提供 `nanmax`）。
- `a += a.T` 这类**输入输出内存重叠**的原地运算如何处理（先物化临时量或明确 panic）。

**绘图（P1）**

- 格式字符串 `plt.plot(x, y, 'rx')`——课程里用得非常多。
- `figure(figsize=)`；`fig, ax = plt.subplots(1, 3)` 返回元组与 Axes 数组。
- `colorbar`、`contourf`、`axhline/axvline`、`text/annotate`、对数坐标（老版 ex8 用对数级 levels）。
- **3D 曲面**：C1 代价曲面 lab 用 `plot_surface`，需明确做还是排除（建议排除，用 contour 替代并说明）。

### 12.5 测试与工程缺口（P1）

1. **绘图无法在全量回归里验收**：`show()` 会阻塞一个 GUI 窗口，CI/无头环境跑不了。建议：
   - 断言层面只比较序列化后的 **figure spec JSON**，不开窗口；
   - 增加几乎零成本的 **`savehtml(path)`**（本来就在生成 HTML），作为无显示器出口；
   - 提供后端开关（如环境变量 `KOALA_PLOT_BACKEND=html|webview`）；
   - 另需验证：**同一进程内多次 `show()`** 时 webview 能否反复创建/运行（未核实）。
2. **随机数可复现性**：部分课程参考值依赖 `np.random.seed(1)` 的结果。
   自研 Box-Muller 不可能与 NumPy 的 MT19937 序列一致。二选一：
   （a）实现与 NumPy legacy `RandomState` 兼容的生成器；
   （b）验收用例避开依赖随机序列的参考值（**推荐，成本低**）。
3. **测试命令不一致**：原文 §8 写 `build-and-test.sh`，实际是
   `./build-debug.sh` + `./test-debug.sh`（已更正）。
4. **参考值版本混用**：§1.1 以 2022 版为基准，但 M2 的 J≈32.07、θ≈(-3.63,1.17)
   来自老版 ex1；2022 版 lab 用 w=2、b=1 初始化。必须选定一个版本（已在 §8 M2 标注）。
5. **并发语义未定义**：项目已有 `Koala_Concurrency_Plan.md`。
   Tensor storage 跨任务共享、view 被并发改写时的行为没有规定。
   **v1 至少写明"不保证线程安全"**，并在手册中说明。
6. **交付物缺失**：应补 `docs/koala_tensor_api.md`、`koala_np_api.md`、`koala_plot_api.md`
   （与既有 `koala_*_api.md` 命名一致），以及一份 **NumPy → Koala 移植对照表**
   （含差异清单：`__div__`、掩码用 `&`、无 `**`、无 fancy indexing 全集等）。

### 12.6 文档结构问题（P2）

1. **§11 篇幅过大**：GPU/NPU/LLM 约占全文三分之一，冲淡 v1。
   建议拆到 `docs/koala_ml_roadmap.md`，主文档只留依赖图与一句话指引。
2. **"不预留任何代码结构"过于绝对**（§4.16、§11 开头）：
   建议改为"**不写未来代码，但不堵死**"。例如 storage 留一个 allocator/后端指针、
   dtype 用可扩展枚举，代价几乎为零；§11.2 与 §11.9.1 其实已默认依赖这种分层。
3. **时效性内容混入设计**：如"核实于 redefined 分支工作区""ggml 1.0.0 于 2026-11-13 发布"。
   建议注明来源与核实日期，或移入附录，避免正文过期。
4. **缺少"为什么内置而不是独立包"的论证** —— 本轮已由 §13.1 补齐（结论：必须内置）。

### 12.7 修订优先级汇总

| 优先级 | 修订项 | 落点 |
|---|---|---|
| P0 | dtype 静态化方案拍板（A/B） | §13.2、§14 |
| P0 | 标量在左的编译器分支 | §13 T1 |
| P0 | 重写 §1.3 为逐作业矩阵 | §12.3 |
| P0 | `__eq__` 返回类型 / 真值语义 spike | §13 T0-a |
| P1 | 打印、`allclose`、`meshgrid`、逻辑位运算、`norm/det`、`.npy`、一维整数索引 | M1/M2 |
| P1 | 绘图测试策略 + `savehtml` + 后端开关 | M3 |
| P1 | 随机数兼容策略、参考值版本、测试命令 | §8、§12.5 |
| P2 | 拆分 §11、补 API 文档与移植对照表 | M5 |

---

## 13. Tensor 编译器内置：决策、方案与 M0 起点

**用户已拍板**：Tensor 放进 `std/builtin`，并由**编译器内置支持**，以便做更多优化。
本章记录决策依据、必须先定的设计选择，以及比吴恩达课程更小的起点 M0。

### 13.1 为什么必须内置（代码依据）

不只是性能问题——**没有编译器支持，M0 的自然写法根本不成立**。

| 依据 | 位置 | 含义 |
|---|---|---|
| 二元运算只看左操作数类型，无反向钩子 | `parser_expr.c:4020 parse_binary`（4136 行起）；`number.kl: float64.__mul__(rhs float64)` | `2.0 * t` 直接类型报错，**库层无法自救**；只有编译器能加"rhs 是 Tensor"的分支 |
| `@intrinsic` 仅限 builtin 包 | `parser.c:1461` | 放 `std/builtin` 才能像 `float64` 那样把运算符声明为 intrinsic，由编译器直接下降 |
| 非内置类型走动态分发 | `isel.c:121/323` → `OP_NUM_*` → `vm_ops.h:1647` → `kl_call_slot(SLOT_ADD)` | 每次运算都查表；内置后可加专用 opcode，省掉分发 |
| irgen 已有按类型特化先例 | `irgen.c:1030`（str/range/tuple/list） | 新增 `type_is_tensor` + `emit_tensor_*` 是既有套路 |
| 序列已有专用 opcode | `opcode_list.h:1936/1965 OP_SEQ_GET/SET` | `OP_TENSOR_GET/SET` 有现成参照 |
| native 对象模型完备 | `bytebuf.c:413`、`modules/builtin.c:274` | storage/释放钩子照搬 Buffer |

**内置能带来的优化（按收益排序）**：
1. **逐元素融合**（T3）：`x*w + b - y` 合成一次遍历，消掉中间 Tensor；
   `(err*x).mean()` 融合成"逐元素 + 归约"一趟。收益最大，风险也最大。
2. **专用 opcode + JIT 内联标量读写**（T2）：消除 slot 查表；元素类型静态已知时
   `a[i]` 可内联为地址计算 + load/store。
3. **编译期 dtype/shape 检查与提升推导**（T1）：错误在编译期报出，运行期少一层判断。
4. 远期：SIMD/多线程/设备后端（§11.7、§11.9）。

### 13.2 必须先拍板：dtype 放静态类型还是运行时

| 方案 | 写法 | 优点 | 缺点 |
|---|---|---|---|
| **A：泛型 `Tensor[T]`（推荐）** | `Tensor[float64]`、`Tensor[int64]` | `a[i]`、`sum()` 的返回类型就是 `T`，§12.2 的"多态返回"问题根治；编译期已知元素类型，可特化内核、JIT 内联；类型提升（`Tensor[int64] + Tensor[float64]` → `Tensor[float64]`）在编译期算出 | 与 NumPy 的运行时 `dtype=` 不完全同形；`astype` 需写成 `astype[float32]()` 之类由编译器特判的形式 |
| B：非泛型 `Tensor` + 运行时 dtype 字段 | 更像 NumPy | Python 移植更直接 | 编译器不知元素类型，`a[i]` 只能返回 `any`/联合类型，T2/T3 的多数优化做不了 |

**建议选 A**：Koala 是静态类型语言，内置的价值正来自"编译期就知道类型"。
M0 只实现 `Tensor[float64]`，并让裸写的 `Tensor` 默认等于 `Tensor[float64]`
（**依赖泛型默认参数，T0-a spike 必须确认；若不支持则要求显式写 `Tensor[float64]`**）。
`koala/np` 门面继续提供 `dtype=` 写法并转换到对应泛型类型，兼顾移植体验。

**连带影响**：选 A 后，0 维 Tensor 的必要性下降（标量返回 `T` 已经是静态明确的），
但 §11.2 的 `loss.backward()` 仍需要 0 维 Tensor，届时按 `Tensor[T]` 加 `shape=()` 即可，不构成破坏性变更。

### 13.3 M0 目标与验收

**目标**：只用一维 float64 Tensor，跑通梯度下降拟合直线。
数据是合成的 `y = 2x + 1`，从 `w=0, b=0` 出发，结束时断言 `w≈2`、`b≈1`。

不需要绘图、CSV、二维、广播、切片、线代；但已覆盖 native storage、逐元素运算、
归约、运算符重载、编译器下降、VM/JIT 双模式这条主链路。

验收脚本形态（示意，最终语法以 T0-a spike 结论为准）：

```kl
let x = Tensor.linspace(0.0, 10.0, 50)
let y = 2.0 * x + 1.0
var w = 0.0
var b = 0.0
for i in 0..2000 {
    let err = x * w + b - y
    w -= 0.01 * (err * x).mean()
    b -= 0.01 * err.mean()
}
assert isclose(w, 2.0) && isclose(b, 1.0)
```

### 13.4 M0 分层：T0-a → T0 → T1 → T2 → T3

每层独立编译、测试、汇报。每层结束都要 `./build-debug.sh` + `./test-debug.sh` 全绿，
**JIT 与 --no-jit 双模式都过**，且内存全释放。遇到崩溃/断言失败立即停下报告，不擅自绕过。

#### T0-a — 语言能力 spike（不写 Tensor，只回答问题）

用一个十几行的玩具类验证正式设计依赖的前提，产出一页结论写入本文档或
`koala_usage_issues.md`：

| 待验证 | 为什么必须先弄清楚 |
|---|---|
| 泛型类能否有默认类型参数（`Tensor` ≡ `Tensor[float64]`） | 决定 §13.2 方案 A 的书写形态 |
| `__eq__` 能否返回非 bool；`if obj:` 的真值规则；dict/set 是否依赖 `__eq__` | 决定比较运算与布尔掩码的可行性 |
| 变长 `__getitem__(idx ...)` 接收 int/slice/Tensor 混合实参的类型推断 | 决定多维索引 API 形态（M1 才用，但结论影响 T0 的声明） |
| 具名参数 + 默认值在 native 方法上的行为 | 已核实语法支持，需确认 native 侧参数解析 |
| 照搬 Buffer 的 native 对象在 JIT/VM 下释放是否干净 | 决定 storage 生命周期方案 |

#### T0 — 最小 Tensor 运行时（不动编译器）

- **运行时**：新增 `src/objects/tensorobj.c` + `include/runtime/tensorobj.h`。
  内存结构 = storage（引用计数）+ dtype + shape + strides + offset；
  **M0 只用一维，但结构不限维数**（避免后续重写存储层）。
  在 `src/modules/builtin.c` 注册 `tensor_type`，`_free` 钩子释放 storage。
- **`std/builtin`**：新增 `libs/std/builtin/tensor.kl`，`@native` 声明：
  - 创建：`array(list)`、`zeros(n)`、`ones(n)`、`arange`、`linspace`
  - 访问：`len()`、`a[i]`（int 读写，含负索引，越界 panic）
  - 归约：`sum/mean/max/min`（M0 返回 float64 标量）
  - ufunc：`exp/log/sqrt/abs`（复用 math 的 C 实现）
  - 输出：`tolist()`、`__str__`、`allclose(a, b)`
- **运算符**：先声明普通 dunder（`__add__ __sub__ __mul__ __div__ __neg__ __iadd__ ...`），
  走现有 `OP_NUM_*` + slot 路径。**本层只要求 `t * 2.0` 通，`2.0 * t` 暂不通**。
- **错误语义**：形状不一致、越界一律 panic，消息含操作名与双方 shape/dtype。
- **验收**：ut 覆盖每个 API 与每个 panic 分支（`@should_panic`）；内存全释放。

#### T1 — 类型检查器内置（解决语义问题）

- `include/parser/typespec.h` 增加 `type_is_tensor`。
- `parser_expr.c: parse_binary` 新增分支：**lhs 为数字类型且 rhs 为 Tensor 时，
  改由 Tensor 侧解析**，对 `-` `/` 生成"反向"语义的调用（`2.0 - t` ≠ `t - 2.0`）。
  **只新增分支，不改既有路径**（与用户 switch WIP 同文件不同函数）。
- 复合赋值 `t += ...` / `w -= ...` 的解析确认走到 `__iadd__`/`__isub__`。
- 编译期错误用例：dtype 不匹配、向 Tensor 传入非数字参数、非法运算组合。
- **验收**：§13.3 的 demo 以自然写法 `0.01 * err.mean()` 通过类型检查并运行。

#### T2 — 专用 opcode + JIT 快速路径

- `include/common/opcode_list.h` 新增 `OP_TENSOR_ADD/SUB/MUL/DIV`
  （Tensor⊕Tensor、Tensor⊕标量两种形式）与 `OP_TENSOR_GET/SET`（标量下标）。
- `isel.c` 按 `type_is_tensor` 选这些 opcode，不再退回 `OP_NUM_*`；
  `vm_ops.h` 直接调 C 内核；同步更新 `lower_operands.c`、`klr/printer.c`、klc 读写。
- JIT：`OP_TENSOR_GET/SET` 内联为 offset/stride 地址计算 + load/store
  （元素类型由 `T` 静态确定）；其余 Tensor opcode 先走 helper。
- 同步更新 JIT `DESIGN.md` 的 opcode 全量核对表。
- **验收**：bench 三方对照——T0 的 slot 路径、T2 的专用 opcode、纯 Koala list 循环；
  数字写入 bench 结果，作为继续投入 T3 的依据。

#### T3 — 逐元素融合（内置的最大收益，单独立项）

- 新增优化 pass（如 `src/parser/opt/tensor_fuse.c`）：
  - `x * w + b - y` 合并为一个循环，不产生 3 个中间 Tensor；
  - `(err * x).mean()` 融合为"逐元素 + 归约"一次遍历，不产生临时数组。
- **安全前提**：只融合中间结果**无逃逸、无别名、未被原地修改**的纯表达式；
  不满足就不融合。view 语义（M0.5 引入）会显著增加别名分析难度，因此 T3 必须在 view 落地后重新评估。
- **风险最高**：等 T0–T2 稳定、bench 证明临时对象确为瓶颈后再做；**M0 验收不依赖 T3**。
- **验收**：融合路径与未融合路径逐元素对照的 ut；bench 显示中间分配次数与耗时下降。

### 13.5 受影响文件一览（T0–T2）

| 层 | 文件 |
|---|---|
| 库 | `libs/std/builtin/tensor.kl`（新建）+ 包内 `test/` 新增 ut |
| 运行时 | `src/objects/tensorobj.c`、`include/runtime/tensorobj.h`（新建）；`src/modules/builtin.c`；`src/CMakeLists.txt` |
| 类型系统 | `include/parser/typespec.h`；`src/parser/parser_expr.c`（`parse_binary`） |
| IR / 后端 | `include/common/opcode_list.h`；`src/parser/irgen.c`；`src/parser/backend/isel.c`、`lower_operands.c`；`src/parser/klr/printer.c`；klc 读写 |
| VM / JIT | `include/runtime/vm_ops.h`；`src/jit/jit.c`；`src/jit/DESIGN.md`（核对表） |
| 基准 | `bench/` 新增 Tensor 项 |
| 文档 | 本文档；后续 `docs/koala_tensor_api.md` |

### 13.6 M0 专属风险

| 风险 | 缓解 |
|---|---|
| `parser_expr.c` 与用户 switch WIP 冲突 | 开工前确认工作区状态；T1 只加新分支；改动前后跑全量回归 |
| 新增 opcode 破坏 klc 兼容 / JIT 核对遗漏 | T2 单独成层，便于回滚；klc 读写、printer、isel、JIT 核对表同批更新 |
| 泛型默认参数不支持 | T0-a 先验证；退路是要求显式 `Tensor[float64]`，np 门面负责简写 |
| `__eq__` 不能返回 Tensor | T0-a 验证；退路是比较用 `eq/lt/...` 函数式 API，运算符比较后置（会影响掩码写法，需上报） |
| storage 引用计数与 view 生命周期 | M0 无 view，风险低；M0.5 引入 view 时按 Buffer 先例做专项 ut |
| T3 融合改变数值结果（浮点结合顺序） | 融合只做"相同运算顺序的循环合并"，不重排浮点归约顺序；如需重排必须显式开关 |

### 13.7 M0 之后的衔接

```
M0（一维 float64，拟合直线，T0-a→T0→T1→T2[→T3]）
  └─ M0.5（二维 + 广播 + 切片 view + axis 归约 + dot/matmul/.T）
       └─ M1（其余 dtype、比较/布尔 Tensor、完整 ufunc、形状操作、打印、allclose）
            └─ M2（loadtxt/.npy、random、np 门面）→ 第一个真实课程作业
                 └─ M3（koala/plot）→ M4（高级线代）→ M5（收尾）
```

**刻意不做、且以后补上不会破坏兼容的部分**：

| 暂不做 | 以后加上是否兼容 |
|---|---|
| 切片与 view | ✅ M0 不提供切片，就不存在"先拷贝后改 view"的破坏性变更 |
| 二维与广播 | ✅ 存储结构已通用，同形状只是广播的特例 |
| 比较运算与 bool Tensor | ✅ 新增即可（受 T0-a 的 `__eq__` 结论约束） |
| float32/int64/bool | ✅ 方案 A 下就是新的 `Tensor[T]` 实例化 |
| `koala/np`、plot、loadtxt | ✅ 外围薄封装 |

**唯一不能拖的决定**：§13.2 的 dtype 方案（A/B）。它决定 `sum()`/`a[i]` 的返回类型形态，
定错以后就是破坏性返工——这也是把它列为 P0 的原因。

---

## 14. 确认事项与决策记录

### 14.1 已确认（用户已拍板）

| # | 决策 | 影响 |
|---|---|---|
| 1 | **Tensor 内置于 `std/builtin`，并由编译器内置支持** | §4.1、§7、§10、§13 已按此修订；原"parser/编译器零改动"作废 |
| 2 | **起点收敛为 M0**（比吴恩达课程更小的一维拟合直线任务） | §8 新增 M0/M0.5；原 M1 拆分为"补齐"角色 |
| 3 | 绘图以 webview 为后端；不引外部 BLAS/LAPACK；不加 `@` 运算符 | 维持原方案 |
| 4 | **plot v1 spec/几何层先用 list 装数据，Tensor 入口后置（additive）**；marching squares 与 layout 数学移 Koala 侧、渲染器哑画 | §4.15/§6.1/§6.2/§6.3 已按此修订；Tensor 落地后加 Tensor 入口，几何层不改 |

### 14.2 待确认（阻塞开工，按优先级）

| # | 待确认 | 选项 | 建议 | 阻塞什么 |
|---|---|---|---|---|
| 1 | **dtype 方案** | A：泛型 `Tensor[T]` / B：运行时 dtype | **A** | 阻塞 T0（API 形态） |
| 2 | **M0 推进粒度** | 只做 T0 后汇报 / T0–T2 连续推进每层停一次 | 先 T0-a + T0，再定 | 阻塞排期 |
| 3 | `__eq__` 返回 bool Tensor 是否可行 | 由 T0-a spike 回答；不可行则退到函数式比较 | 先 spike | 影响掩码写法 |
| 4 | 课程基准版本 | 2022 版 MLS / 老版 Octave | 2022 版为主，老版作扩展 | 阻塞 M2 断言值 |
| 5 | 门面包名 | `koala/np` / `koala/numpy` | `koala/np` | M2 |
| 6 | 0 维 Tensor | 做 / 不做 | 与第 1 项联动，A 方案下可不做 | M1 |
| 7 | `savefig` / `savehtml` 时机 | M3 / 后置 | `savehtml` 提前到 M3（测试需要），`savefig` 后置 | M3 测试策略 |
| 8 | 是否拆分 §11 到独立 roadmap 文档 | 拆 / 不拆 | 拆 | 文档维护 |

### 14.3 变更记录

- 本轮：新增 §12（评审）、§13（内置决策 + M0）、§14（确认事项）；
  修订 §4.1、§4.7、§7、§8（M0/M0.5/M1/M2、测试命令）、§9（风险与开放问题）、§10（范围）。
- 2026-11-21：用户拍板 plot **spec-first，v1 用 list 装数据**（Tensor 落地后加入口，additive，
  几何层不改）；marching squares + layout 数学移 Koala 侧、渲染器哑画（§6 两偏差签字）。
  修订 §4.15/§6.1/§6.2/§6.3/§14.1。plot spec 层开始实现（Tensor 部分仍为设计）。
- 本文档 Tensor 部分仍为**设计方案**，未产生实现代码改动；plot spec 层除外（已开工）。
