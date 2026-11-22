# kparse TODO

Koala 无损（保留注释）CST 解析器，基于 `koala/klex`，文法权威为 `src/parser/koala.y`。

## v1 现状（已完成）

- `wombat build libs/koala/kparse` 编译通过。
- 711 文件语料库实测：**parse_fail=13，loss_fail=0**（其余全部字节级 round-trip `text()==src`）。
- 13 个失败全部是「带类型参数的 lambda」（`expected '|'`），其余语法构造均已支持。
- 词法侧（klex）完整：全部关键字、char/bytes 字面量、三引号文本块、`#`/`//`/`/* */` 注释、
  `__KOALA_X__` 占位符、`{|`/`{||`、`?.`/`!.`/`...`/`??`。
- 已支持：switch/case（`case in [...]`/`case is T`/`case _`/多模式/case_tail）、enum、link、
  from-import、import-as、type alias、class/trait（extends `&`、`class tuple[infer T]`、
  `class str` 重开、static/pub 成员、无初始化字段、trait 方法签名）、func（块体/`= expr` 体/原型、
  泛型 `[T:Bound]`/`const N int`/`infer T`、变参 `args...T`、带类型默认值）、let/var/const +
  元组解构 + let-else、if/while-let（含括号写法）、loop、for（`_`/`(k,v)`）、with（含 `with let ... else`）、
  return/break/continue、全部赋值运算符 + 元组赋值、完整 Pratt 优先级、`?.`/`!.`/后缀 `!`/`?`、
  切片全写法（含 step）、多维索引、list/map(`{:}`)/tuple/paren 字面量、注解各形式、全部类型关键字。

## v2 计划

### P0 · 补齐唯一硬缺口（必做，验收线 parse_fail=0）

- [ ] **带类型参数的 lambda** `{|x int| ...}` / `{|x int, y str| ...}`。
  - 根因：`parse_lambda_param()` 用 `at_type_start()+parse_type()`（含 union），把闭合 `|`
    当作 union 分隔符吞掉。
  - 修法（助手已就绪）：改用 `at_base_type_start()+parse_base_type()`。
  - 无类型 `{|x| ...}` 与零参 `{|| ...}` 已正常，勿回归。
  - 失败文件参考：`libs/std/itertools/test/{test_map,test_filter,test_reduce,test_flat_map,test_take_while}.kl`、
    `test/test-run/{test_lambda_forms,test_lambda_infer,test_lambda_newline_semi,test_lambda_no_capture,
    test_inline_closure,test_func_value_ssa_merge,test_tailcall_other,test_unroll}.kl`。

### P1 · 测试与工程化（可回归）

- [ ] 建 `libs/koala/kparse/test/`，沿用 klex 约定（`__pkg__.toml` + `test_*.kl` + `ut`）：
  - [ ] golden sexpr：各语法构造的树形快照。
  - [ ] round-trip：`text()==src`（含注释/三引号/CRLF 边界）。
  - [ ] 错误恢复 + 诊断位置断言。
- [ ] 接 `wombat test libs/koala/kparse`，纳入 `./build-std-koala.sh` 回归。

### P2 · 错误恢复与诊断质量

- [ ] 优化 panic-mode 同步点，减少级联报错（一处失误连带多条 `expected '}'`/`expected an expression`）。
- [ ] 诊断带 expected/found 与上下文，定位更准。

### P3 · 节点 API（为工具链铺路）

- [ ] 通用 visitor/遍历、`loc()`/span 查询、子节点便利访问器，避免上层手写递归。

### 可选 · 文法保真收紧（取决于用途）

v1 为无损故意「宽松」之处，按 `koala.y` 收紧：

- [ ] `is`/`as` 右侧不吞 union（文法 `type_opt` 不含 union；`x is A | B` 应为 `(x is A) | B`）。
- [ ] 类型实参 / 函数类型参数与返回 / 数组元素不吞 union（`base_type`/`optional_type`/`type` 分层）。
- [ ] const 类型限定为 `const_type`（原始类型 + `str` + tuple）。
- [ ] if 表达式仅限 `value_expr` 位置（let/var 初始化、`=`、return、`= expr` 函数体）。
- [ ] 注解参数限定（原始类型列表 / 字符串字面量 / `name=int|str`）。
- [ ] 修饰符位置约束（`static` 仅方法、trait 名仅 `ID|ANY`、裸 `array` 拒绝、多注解等）。

> 用途权衡：目标为**校验器**则收紧；目标为 **fmt/IDE 容错**则保持宽松更好。先定用途再决定。

### 跨组件议题（单列）

- [ ] **CRLF 字节无损**：需 klex 保留裸 `\r`，关联编译器 issue #26（按设计当前不修）。
- [ ] **性能/GC**：大文件/批量解析的内存与速度优化。

## v3 方向（CST 落地应用）

- formatter（迁移现有 `tools/kl-fmt.py`）。
- doc 生成（`tools/kl-doc.py`）。
- lint / 语义检查前端。
- IDE 语言服务（补全/跳转/悬停，需再加符号解析层）。

## 备注

- Koala 脚本中 `func main()` 会被自动执行，**不要**在末尾再显式调用 `main()`（会跑两遍，曾致语料库驱动 GC OOM）。
- 编译与测试勿在同一并行批次执行（陈旧 `.klc` 会致假崩溃）；耗时命令后台执行并轮询日志。
