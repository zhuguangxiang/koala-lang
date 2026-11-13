# test-bench

性能基准测试，使用 `@bench` 注解，不参与 lit / ctest。

## 如何使用

### 编写

```koala
import "std/ut"

func work() int {
    var s = 0
    for i in range(15) { s = s + i }
    return s
}

@bench(warmup=3, runs=10)
func bench_work() {
    for _ in range(100000) { work() }
}
```

- 参数：`warmup`（预热次数，默认 0，不计时）和 `runs`（计时次数，默认 1），都必须是整数，
  也都可以省略，例如 `@bench(runs=3)`。
- 被标注的函数必须是顶层的非泛型函数，不能有参数，也不能有返回值。
- 单次调用太短时计时噪声会很大，建议在函数体里加一层外循环，把每次调用拉长到毫秒级。
- 函数体内抛出异常时，该 bench 记为失败，不会出现在结果表中。

### 运行

```sh
source env.sh
koala test/test-bench/<file>.kl ++ --test                      # ++ 之后是编译选项
koala test/test-bench/<file>.kl ++ --test --domtree --unroll  # 加编译选项做对比
```

`--test` 会运行文件里的全部 `@test` / `@bench` 函数，最后输出结果表：

```
Benchmark results:

  Name        Min         Max         Mean        Rounds
  ----------  ----------  ----------  ----------  ------
  bench_work    1.604 ms    2.057 ms    1.781 ms  10
```

- Min / Max / Mean 分别是 `runs` 次计时中单次调用的最短、最长和平均耗时，不包含预热。
- 时间单位按 Max 自动选择 ns / us / ms。
- 要对比两组编译选项时，建议交替多跑几次，以 Mean 为准。

## test_unroll.kl：循环展开

外层 `range(100000)` 不展开，只展开内层小循环。

```sh
koala test/test-bench/test_unroll.kl ++ --test                     # 不展开
koala test/test-bench/test_unroll.kl ++ --test --domtree --unroll  # 展开
```

均值（ms，Debug 构建，交替各跑 3 次）：

| bench | 不展开 | 展开 | 说明 |
|-------|-------:|-----:|------|
| sum15 | 3.3 | 1.75 | 整体折叠为 `ret 105` |
| poly  | 2.8 | 2.2  | 计数器替换为常量 |
| dot4  | 2.5 | 2.3  | 旋转后的 while 循环，体内运算较多 |
| find  | 2.7 | 1.9  | 提前 return，每个副本保留出口 |
| grid  | 4.3 | 1.7  | 4×4 嵌套循环，分两轮展开后折叠 |

- 以 sum15 展开后的 ≈1.75 ms 作为外层循环加调用的基准开销，扣除后的内层耗时：
  sum15 1.6→0，find 1.0→0.15，grid 2.6→0，poly 1.1→0.5，dot4 0.8→0.55。
- 能折叠成常量的场景收益最大；循环体越重，循环控制开销占比越小，收益越小。
- `range(n)` 的头块会进入 n+1 次，而 `UNROLL_MAX_TRIPS` 为 16，所以最多展开 `range(15)`。
