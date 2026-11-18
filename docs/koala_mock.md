# Koala `@mock` 特性设计方案

> 状态:已实现并通过全部测试(2026-11-17)。
> 相关测试:`test/test-kl/test_mock_*.kl`、`test/test-run/test_mock_*.kl`、`test/test-ut/test_mock_annotation.kl`。

## 1. 目标与核心语义

`@mock` 用于单元测试:在 **test 模式**(`koala --test`)下,把一个**跨模块导入**的函数或方法
替换为测试模块中定义的 stub(桩函数),从而隔离外部依赖(环境变量、网络、时间等)。

```kl
import "std/os"

@mock("os.getenv")
func getenv_stub(key str) str? {
    if key == "MOCK_KEY" { return "mocked" }
    return nil
}
```

核心语义(均为确定设计,非临时限制):

1. **仅 test 模式激活**。stub 的收集与 `@test`/`@skip`/`@bench` 一样在加载期无条件进行,
   但只有 `--test` 时才替换导入绑定;正常运行完全不受影响(`test_mock_inactive.kl`)。
2. **模块局部,不传递**。stub 只替换**定义它的模块自己的导入**。其他模块——包括被测库
   内部——对同一目标的调用不受影响。mock 是测试模块的私有环境,不是全局补丁
   (`test_mock_indirect.kl`)。
3. **一个目标只允许一个 stub**。同一模块内对同一目标(如 `os.getenv`)重复 mock 是编译错误
   (`test_mock_duplicate.kl`)。
4. **stub 必须自包含,禁止引用任何被 mock 的目标**——自己的或其他 stub 的。test 模式下
   本模块对被 mock 目标的所有导入都绑定对应 stub:stub 引用自身目标会直接无限递归;
   引用其他 stub 的目标则把 stub 耦合起来,可能形成 A→B→A 的递归环。两者都在编译期报错
   (`test_mock_self_call.kl`、`test_mock_cross_reference.kl`)。不提供"委托真实实现"的
   部分 mock 能力——这是明确的取舍:换来机制最简(无特殊导入项、无 klc 标志位、运行期无特判)。
5. **签名必须精确匹配**。返回类型、参数个数、参数类型逐一检查;方法 stub 把接收者显式声明为
   第一个参数(`test_mock_signature.kl`、`test_mock_method_signature.kl`)。
6. **不可 mock 的目标**:trait 方法(经接口表分派,不走导入项)、泛型函数/泛型类的方法
   (单态化生成,无导入项)、本模块内的函数/方法(本地调用不走导入项)。前两者在编译期报错
   (`test_mock_target_errors.kl`)。

## 2. 总体架构

替换发生在**导入解析(import resolution)这一个点**,整条链路分三段:

```
koalac(编译期)                          klc 文件                     koala(运行期)
─────────────────────                  ──────────                   ─────────────────────
注解校验 / 目标解析 / 签名检查            函数注解表携带               加载期:收集 stub 到
        │                              规范化目标 key               ModuleObject.mocks
        ▼                                                                   │
目标规范化为注册 key ──────────────►    "std/os::getenv"                    ▼
"std/os::getenv"                       "pkg::Class::method"         test 模式 resolve_import:
"pkg::Class::method"                                                  用本模块 mocks 重绑定
        │                                                             自己的导入项
        ▼
parser:stub 体内引用任何被 mock 目标
→ 编译错误(禁止递归/stub 耦合)
```

关键决策:**重绑定只发生一次**(模块加载时),把 stub 对象写入模块的函数槽位
(`mo->funcs`)。此后解释器、JIT、函数值(`let f = os.getenv`)全部经由同一槽位取函数,
三条执行路径自动一致,调用时**零额外开销**——没有任何运行期"是否被 mock"的判断。
函数级的 `@mock` 注解是 mock 的**唯一信息源**,不需要任何调用边/导入项级标志。

## 3. 编译期(koalac)

### 3.1 注解校验(`parser.c` 注解检查)

`@mock` 只能用于**顶层非泛型函数**:不得带类型参数、注解不得带类型实参、必须提供目标字符串、
必须有函数体(stub 实现)。目标写法为 `"pkg.func"` 或 `"pkg.Class.method"`。

### 3.2 目标解析与规范化(`check_mock_signature`)

- 包名部分接受三种写法:`import "std/os"` 的默认名 `os`、`import ... as x` 的别名、
  `from "std/os" import getenv` 场景下的包名,以及包全路径 `std/os`
  (`resolve_mock_package`,见 `test_mock_alias.kl`、`test_mock_from_import.kl`)。
- 解析出目标符号后,把目标规范化为**注册 key**:包全路径 + `::` 分隔,即
  `std/os::getenv` 或 `test_pkg/mock_calc_dep::Calc::compute`(`mock_key`)。key 与导入名无关,
  运行期用它匹配导入项,别名/from-import 天然一致。
- 拒绝 trait 方法、泛型函数、泛型类,给出明确诊断。

### 3.3 检查顺序(重要)

**先做全部签名检查,最后做重复目标检查;全部通过后 key 才写入 `stub->ann_val`。**

- 签名非法的 stub 不占用目标:同一文件里多个签名错误的 stub 指向同一目标时,每个 stub
  报出自己的签名错误,而不是被"重复目标"错误掩盖。
- 两个**合法** stub 指向同一目标时,后检查者报重复错误——禁止重复 mock 的语义不变。
- `ann_val` 含 `::` 即表示"已解析的合法 key"(用户原始目标只含标识符与 `.`,不可能有 `:`),
  自引用检查据此识别合法 stub。meta 阶段(`kl_parse_ast_func_meta`)先于函数体解析
  (`kl_parse_ast`)运行,因此解析 stub 体时 key 已就绪。

### 3.4 mock 引用检查(`parser.c::check_mock_reference`)

**引用级前端检查,只挂在 id 和 dot 两处**——函数符号进入表达式的唯一两扇门:

- `parse_ident` 包装函数:裸名引用(from-import 的 `getenv`,`find_symbol` 已解包为原符号);
- `parse_dot` 包装函数:限定名引用(`os.getenv`、`c.compute`)。

两者同构:原函数体改名 `*_inner`,包装层统一挂检查(dot 的成功分支均提前 return,
包装层也保证不漏);检查条件均为 `exp->sym && exp->sym->kind == SYM_FUNC`。

当前函数(`get_current_function`)是合法 stub 时,把被引用函数(`SYM_FUNC`)的全路径 key
(包 `path` + 父类名 + 名字,实例方法经 `origin` 取本体)与**本模块所有合法 stub 的目标**
比对:

```
# 引用自身目标
@mock stub 'getenv_stub' cannot call or reference its own target 'std/os::getenv';
the stub must fully replace the target

# 引用其他 stub 的目标(扫描 pm->stbl 中所有 mock 注解,与 check_mock_duplicate 同模式)
@mock stub 'env_or_stub' cannot reference mocked target 'std/os::getenv'
(mocked by stub 'getenv_stub'); stubs must be self-contained
```

- 检查的是**引用**而非调用:调用、赋值/函数值加载、传参、返回、放入容器等一切用法,
  都在符号解析的那一刻被拦截,与外层运算无关;含 stub 内 lambda 里的引用;
- 被引用符号非外部(`SYM_FLAGS_EXT`)直接跳过——mock 目标必为导入符号,本地函数不可能是;
- 非 stub 函数只付出一次作用域查找 + 注解判断的开销,不拼 key、不扫描;
- 与其他 mock 检查同处前端:`koalac %s` 无需 `--cgen` 即报错,错误定位用引用表达式 loc。

### 3.5 klc 写入

导入项按原样写入(`KlcImport.kind` 即导入种类,无标志位);stub 的注解值即规范化 key,
随函数注解表写入 klc。

## 4. 运行期(koala)

### 4.1 数据结构(`include/runtime/modobj.h`、`src/objects/modobj.c`)

- `ModuleObject.mocks`:`HashMap *`,key 为注册 key,value 为 stub 的 `CodeObject`。
  **按模块持有**,懒创建(无 stub 的模块为 NULL),模块释放时随之销毁。
  接口:`kl_mo_add_mock`(首个注册生效,重复返回 0)、`kl_mo_find_mock`。
- stub 对象本身已在 `mo->funcs` 中,是模块的 GC 根;`mocks` 表只持有指针,不引入额外
  生命周期问题。

### 4.2 加载与激活(`runtime/load_module.c`)

1. **加载期收集**(无条件,与 `@test` 等注解同一遍历):读到 `mock` 注解即
   `_register_mock_target(m, key, stub)` 注册进**本模块**的 `mocks` 表。防御两种陈旧 klc:
   - 目标仍是旧点分格式(不含 `::`):警告并忽略,提示重新编译;
   - 同一 key 已有 stub(编译器已禁止,仅陈旧 klc 可能出现):警告并保留第一个。
2. **导入解析**(`resolve_import`,在收集之后运行):仅当 `kl_get_cmd_opt()->test_mode`
   为真时,对 `IMPORT_KIND_FUNC` / `IMPORT_KIND_METHOD` 条目先查本模块 `mocks`
   (`_lookup_mock_for_import` 用条目的 path/kls/name 拼 key);命中则把 stub 作为解析结果,
   否则照常加载目标模块取真实符号。
3. 解析结果写入 `e->address` 并追加到 `mo->funcs` 槽位。**解释器与 JIT 都从该槽位取函数**
   (解释器 `vm_ops.h` 的 `fn = ENTRY(local_index)`;JIT 在编译时把 `ENTRY(payload)` 解析为
   函数指针嵌入机器码,发生在重绑定之后),因此 mock 对两条执行路径同时生效
   (`test_mock_method.kl` 验证方法重绑定;JIT 路径由共享槽位机制保证,另有专用 JIT 测试覆盖)。

### 4.3 模块局部语义的实现与意义

- 每个模块有独立的 `import_table` 与 `mocks` 表,重绑定是纯模块局部操作:
  **无全局注册表、无加载顺序依赖、无跨测试污染**。早期原型曾用全局 mock 符号表,
  已废弃——全局表会让多个测试模块对同一目标的不同 stub 相互覆盖,且库内部调用被意外替换。
- 库模块(如 `mock_calc_dep`)内部对 `os.getenv` 的调用走它自己的导入项,测试模块的 stub
  与之无关(`test_mock_indirect.kl` 断言库内调用拿到真实值)。
- 推论:若要隔离"被测库对第三方依赖的调用",stub 必须定义在**被测库模块自身**中;
  测试模块无法隔空替换库的内部依赖。这是明确的语义边界,不是缺陷。

## 5. 方案评估

### 5.1 优点

| 方面 | 说明 |
|---|---|
| 零调用开销 | 重绑定只在导入解析时发生一次,调用路径无任何 mock 判断 |
| 执行路径一致 | 解释器/JIT/函数值共享函数槽位,不存在"JIT 绕过 mock"问题 |
| 测试隔离 | 模块局部、无全局状态;多个测试模块可各自 mock 同一目标互不影响 |
| 错误前置 | 目标不存在、trait/泛型、签名不符、重复目标、stub 引用被 mock 目标全部编译期报错 |
| 机制最简 | 函数级注解是唯一信息源;每个符号一条导入项,klc 无标志位,运行期无特判 |
| 导入名无关 | 规范化 key 用包全路径,别名/from-import 自动一致 |

### 5.2 已知边界(按设计接受)

- **不传递**:mock 只作用于定义 stub 的模块(见 4.3)。
- **不支持部分 mock/委托**:stub 调用自身目标是编译错误;目标在测试环境下不可用时,
  stub 全量伪造其行为。需要"按需失败"类测试(如 `Koala_TODO.md §62` 的 redis 场景)时,
  由 stub 按输入条件返回伪造的成功/失败结果。
- **经辅助函数的间接引用不检测**:检查只覆盖 stub 体内的直接引用(调用、函数值加载等)。
  stub 调用本模块辅助函数、辅助函数体内再引用被 mock 目标时,辅助函数不是 stub,检查放行;
  该路径的导入同样绑定 stub,运行期可能间接递归。是否绕回目标由测试作者负责。
- **本模块调用不可 mock**:本地函数/方法调用不经导入项;`@mock` 目标必须是导入符号。
- **trait 方法与泛型不可 mock**:分派机制不同(接口表/单态化),编译期拒绝。
- **签名精确匹配**:`type_spec_equal_exact`,不做协变/逆变放宽。
- stub 只在**顶层函数**上声明;方法 stub 以普通函数形式显式声明接收者。

### 5.3 兼容性注意事项

- klc 的 `KlcImport.kind` 保持纯净枚举,无标志位,新旧格式一致。
- 开发过程中曾短暂引入 `KLC_IMPORT_NOMOCK`(kind 高位 0x80)方案,已整体移除;
  若本地留有该中间版本编译的 klc,重新编译即可(运行期会把 0x80|kind 当作未知种类)。
- 旧点分格式目标的 klc 在运行期警告并忽略(见 4.2),不会静默错绑。

## 6. 使用示例

### 6.1 函数 mock

```kl
import "std/ut"
import "std/os"

@mock("os.getenv")
func getenv_stub(key str) str? {
    if key == "MOCK_KEY" { return "mocked" }
    return nil
}

@test
func test_env() {
    ut.assert_eq(os.getenv("MOCK_KEY") ?? "none", "mocked")
}
```

### 6.2 方法 mock(接收者显式为第一参数)

```kl
import "test_pkg/mock_calc_dep"

@mock("mock_calc_dep.Calc.compute")
func compute_stub(c mock_calc_dep.Calc, x int) int {
    return 42   // 真实 compute(x) 返回 x * 2
}

@test
func test_method() {
    let c = mock_calc_dep.Calc()
    ut.assert_eq(c.compute(3), 42)
}
```

### 6.3 禁止的写法(编译错误)

```kl
@mock("os.getenv")
func getenv_stub(key str) str? {
    return os.getenv(key)   // 错误:调用自身目标,test 模式下会无限递归
}
// error: @mock stub 'getenv_stub' cannot call or reference its own target 'std/os::getenv';
//        the stub must fully replace the target
```

### 6.4 运行

```bash
koala --test test_file.kl        # mock 激活
koala test_file.kl               # mock 不激活,调用真实函数
```

## 7. 测试矩阵

| 测试 | 覆盖点 |
|---|---|
| `test-ut/test_mock_annotation.kl` | 基本拦截:本模块导入被 stub 替换 |
| `test-run/test_mock_alias.kl` | `import ... as` 别名下目标解析 |
| `test-run/test_mock_from_import.kl` | `from ... import` 下目标解析 |
| `test-run/test_mock_inactive.kl` | 非 test 模式 mock 不生效 |
| `test-run/test_mock_method.kl` | 跨模块具体方法重绑定 |
| `test-run/test_mock_indirect.kl` | **模块局部语义**:库内部调用不被 mock |
| `test-kl/test_mock_self_call.kl` | **禁止引用自身目标**:函数调用、方法调用、函数值加载均编译错误 |
| `test-kl/test_mock_cross_reference.kl` | **禁止引用其他 stub 的目标**:stub 必须自包含 |
| `test-kl/test_mock_signature.kl` | 函数签名不符:返回类型/参数类型/参数个数/目标不存在 |
| `test-kl/test_mock_method_signature.kl` | 方法签名不符:接收者/参数类型/返回类型/参数个数 |
| `test-kl/test_mock_duplicate.kl` | 同一目标两个合法 stub → 编译错误 |
| `test-kl/test_mock_target_errors.kl` | trait 方法、泛型目标 → 编译错误 |

依赖包:`test/test_pkg/mock_calc_dep/`,由 `test/build.sh` 统一构建(与其他 test_pkg 包一致);
测试直接 `import "test_pkg/mock_calc_dep"`,RUN 行无需任何依赖编译或 KOALA_PATH 设置。

## 8. 涉及文件

| 文件 | 职责 |
|---|---|
| `src/parser/parser.c` | 注解校验、目标解析、签名/重复检查、key 规范化、mock 引用检查 |
| `src/parser/parser_expr.c` | `parse_ident` / `parse_dot` 挂接 mock 引用检查(仅函数符号) |
| `src/parser/write_klc.c` | 注解(目标 key)与导入项写入 klc |
| `include/runtime/modobj.h` / `src/objects/modobj.c` | `ModuleObject.mocks`、增查接口与释放 |
| `src/runtime/load_module.c` | stub 收集、test 模式导入重绑定、陈旧 klc 防御 |

## 9. 结论

方案以"导入项一次性重绑定"为核心,获得零调用开销、解释器/JIT 天然一致、模块局部无全局状态
三大收益;**mock 限定在当前模块、不传递**,**stub 自包含、禁止引用任何被 mock 目标**,两条语义都由
编译期强校验保证。函数级 `@mock` 注解是唯一信息源,klc 与运行期均无 mock 专用标志或特判,
机制保持最简。