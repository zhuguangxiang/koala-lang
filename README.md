# 🐨 Koala Programming Language

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Language](https://img.shields.io/badge/implementation-C23-orange.svg)
![JIT](https://img.shields.io/badge/JIT-Drop%20Bear%20v1-brightgreen.svg)
![PRs](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)

**Koala is a tiny, fast, statically‑typed language with Python‑like ergonomics — a register VM and an aggressive optimizing compiler written in pure C, with zero dependencies.**

It is built on three non‑negotiable principles:

1. **Python in a statically‑typed language** — the intuition of Python (insertion‑ordered dicts, dunder protocols, convenience methods), guarded by strong static types at compile time.
2. **No hidden corners** — builtin types are declared in readable standard‑library source; compiler behavior is visible, never magical.
3. **Simplicity is mandatory** — simplicity constrains both the API surface *and* the compiler implementation. No hidden machinery.

## A taste of Koala

Every example below is runnable — they live in [`demo/`](demo): `koala demo/tour.kl`, `demo/generics.kl`, `demo/traits.kl`, `demo/lambdas.kl`.

```kl
// Generics with trait bounds — the type argument is inferred at the call site.
func max[T : Comparable](a T, b T) T = if a >= b { a } else { b }

// Classes with operator overloading via dunder syntax hooks.
class Point {
    var x int = 0
    var y int = 0
    func __init__(_x int, _y int) { x = _x; y = _y }
    func __add__(o Point) Point = Point(x + o.x, y + o.y)
    func fmt(f Formatter) { f.write_fmt("Point({}, {})", x, y) }
}

// Null safety: a T? unwraps automatically in the branch where it is present.
func describe(msg str?) str {
    if msg == nil { return "nothing" }
    return msg          // msg is str here, not str?
}

func main() {
    print(max(3, 7))                  // 7
    print(max("a", "b"))              // b
    print(Point(1, 2) + Point(3, 4))  // Point(4, 6)
    print(describe(nil))              // nothing

    for (n, s) in [(1, "a"), (2, "b")] { print(n, s) }   // tuple unpacking
}
```

Generics are **monomorphized** — `Box[int]` and `Box[str]` become concrete types at compile time. Optionals bind with `if let`, and `with` scopes a resource between `__enter__` / `__exit__`:

```kl
class Box[T] {
    var value T
    func __init__(_value T) { value = _value }
    func get() T = value
}

func unwrap_or(v int?, d int) int {
    if let x = v { return x }   // x is int, bound only in this branch
    return d
}

class Guard : Withable {
    func __enter__() { print("enter") }
    func __exit__() { print("exit") }
}

func main() {
    let bi = Box(42)            // Box[int]
    let bs = Box("koala")       // Box[str]
    print(bi.get())             // 42
    print(bs.get())             // koala
    print(unwrap_or(nil, 0))    // 0
    with Guard() {
        print("inside")         // enter → inside → exit
    }
}
```

Traits give polymorphism with **static dispatch**: every type carries an intf‑table built at compile time, so a trait call is one indirect slot call — **O(1), no dynamic lookup**, no vtable search.

```kl
trait Shape {
    func area() float
}

class Circle : Shape {
    var r float
    func __init__(_r float) { r = _r }
    func area() float = 3.14159 * r * r
}

class Square : Shape {
    var side float
    func __init__(_s float) { side = _s }
    func area() float = side * side
}

func total(shapes list[Shape]) float {
    var sum = 0.0
    for s in shapes { sum += s.area() }   // O(1) intf-table slot call
    return sum
}

func main() {
    let shapes list[Shape] = [Circle(1.0), Square(2.0)]
    print(total(shapes))      // 7.14159
}
```

First‑class functions and **expression lambdas**, with parameter types inferred from the expected function type. Lambdas read module globals; capturing locals is on the roadmap.

```kl
type IntFn = func(int) int

func apply(f IntFn, x int) int = f(x)

func main() {
    let inc = { |x int| x + 1 }
    let triple IntFn = { |x| x * 3 }   // x inferred as int from IntFn
    print(apply(inc, 4))               // 5
    print(triple(3))                   // 9
    print(({ |x int| x * 2 })(21))     // 42  — immediately invoked

    let ms = [{ |x int| x + 1 }, { |x int| x * 2 }]
    print(ms[1](10))                   // 20  — lambdas in a list
}
```

## Highlights

**Type system**
- Static strong typing with **local type inference** — annotate only when you want to.
- **Null safety** built in: `T?` optionals, `nil`, automatic narrowing, `if let` / `while let`. No null‑pointer surprises.
- **Generics** with trait bounds and full call‑site inference, monomorphized at compile time.
- **Union types** (`int | str`), tuples with destructuring, and a rich numeric family (`int8`…`int64`, `uint8`…`uint64`, `float16/32/64`, `bfloat16`).
- Classes are final; **traits** are the only subtype relationship, dispatched through a compile‑time intf‑table (O(1), no runtime lookup).

**Ergonomics**
- Python‑like semantics: insertion‑ordered `dict`, `list`, `set`, `str`, `range`, dunder operator protocols.
- **Expression‑oriented**: `if … else …` is a value, `func f(x) = expr` bodies, default & named arguments, multiple returns via tuples.
- **Operator overloading** as syntax hooks — implement `__add__` and `+` just works, no ceremony.
- First‑class functions and expression lambdas; `for` unpacking, `enumerate`, `zip`, `_` placeholders.
- A deliberate error model: **expected failures are return values** (`get_or`, `to_int_or`), **unexpected failures are lightweight panics** — no try/catch ceremony.
- Rust‑style `///` doc comments with a documentation generator.

**Performance & implementation**
- Compact **register‑based bytecode**, primitives live in registers with no heap boxing.
- A real optimizing compiler: **SSA IR**, SCCP / LVN / DCE / constant‑copy propagation / CFG cleanup, instruction selection, **linear‑scan register allocation**, tail‑call optimization and inlining.
- **Drop Bear**, a baseline x86‑64 JIT (v1): hot integer/float/bool functions compile to native code, **up to ~6× over the interpreter** and already competitive with LuaJIT.
- Pure C23, **zero external dependencies**, hackable and easy to extend.

## Quick start

Koala uses standard CMake and has no external dependencies.

```bash
git clone https://github.com/zhuguangxiang/koala-lang.git
cd koala-lang
./build-release.sh          # or ./build-debug.sh
```

This produces the `koala` driver (and the `koalac` compiler).

```bash
./koala bench/sum.kl        # run a source file directly
./koala -c hello.kl         # compile to hello.klc bytecode
./koala hello.klc           # run compiled bytecode
```

Top‑level statements run directly — no `main` required:

```kl
for i in range(1, 6) {
    if i % 2 == 0 { print(i) }
}
```

## Build & packages with wombat

**wombat** is Koala's build system and package manager — written in Koala itself, and installed alongside `koala`. A workspace (`__ws__.toml`) groups packages (`__pkg__.toml`); the standard library and the official libraries are built this way.

```bash
wombat new myapp          # create a workspace (or a package inside one)
wombat build              # compile the workspace / package → ./dist
wombat run main.kl        # run a .kl source file or a .klc artifact
wombat test               # run the tests
wombat check              # check without producing artifacts
wombat fmt                # format source code
wombat doc                # generate documentation
wombat clean              # remove build artifacts
```

Run `wombat help <command>` for details. See the [wombat API reference](docs/koala_wombat_api.md).

## Performance

**Koala vs Lua vs LuaJIT** — hyperfine (mean, lower is better). `Koala` = bare interpreter; `Koala JIT` = Drop Bear baseline JIT (v1); LuaJIT shown with JIT off.

| Benchmark   | Koala     | Koala JIT | Lua      | LuaJIT(-joff) | vs Lua | vs LuaJIT |
| ----------- | --------- | --------- | -------- | ------------- | ------ | --------- |
| sum         | **565.6ms** | 95.9ms  | 1074ms | 956.4ms       | 1.90×  | **1.69×** |
| loop-sum    | 270.5ms   | 67.6ms  | 298.2ms  | 247.7ms       | 1.10×  | 0.92×     |
| fib         | **2.927s**  | 785.7ms | 4.208s | 3.242s        | 1.44×  | **1.11×** |
| ack         | 556.8ms   | 233.1ms | 551.6ms  | 385.5ms       | ~1.00× | 0.69×     |
| tak         | 413.7ms   | 136.0ms | 581.5ms  | 356.1ms       | 1.41×  | 0.86×     |
| shuffle     | **140.7ms** | 42.3ms  | 653.5ms | 461.6ms     | **4.64×** | **3.28×** |
| logic_chain | **1.114s**  | 437.8ms | 1.857s | 1.222s        | 1.67×  | **1.10×** |
| float_arith | **254.5ms** | 178.9ms | 539.5ms | 343.0ms     | **2.12×** | **1.35×** |

Against LuaJIT **with its JIT enabled**, Drop Bear v1 already wins on `tak`, ties on `fib`, and trails on call‑heavy and floating‑point loops — the gap is code quality (the baseline keeps values in the TValue register file), which the next JIT tier targets. Static typing is free ammunition: type specialization, devirtualization and bounds‑check elimination need no guessing and no guards.

### Built‑in benchmarking (`@bench`)

Koala ships its own benchmark harness: annotate a top‑level, no‑arg, void function with `@bench(warmup=N, runs=M)` and run it with `--test`. It reports Min / Max / Mean per function — ideal for tracking JIT tiers without external tools.

```kl
import "std/ut"

@bench(warmup=2, runs=5)
func bench_fib() {
    for _ in range(3) { fib(30) }   // outer loop keeps a round millisecond-scale
}
```

```bash
KOALA_JIT=0 koala test/test-bench/test_jit.kl ++ --test   # interpreter
KOALA_JIT=1 koala test/test-bench/test_jit.kl ++ --test   # Drop Bear JIT
```

[`test/test-bench/test_jit.kl`](test/test-bench/test_jit.kl) covers integer / uint / float loops and self‑ / tail‑recursion. Drop Bear v1 measured ~4× on integer loops and `fib`, ~6.5× on tail recursion, ~1.7× on float loops (the float gap is what Yowie targets). `@bench` also compares compiler options, e.g. `++ --test --domtree --unroll`.

## Under the hood

```
source (.kl)
   │  parser → AST
   ▼
SSA IR  ──►  opt passes (SCCP, LVN, DCE, const/copy prop, CFG, inlining)
   │  instruction selection + linear-scan register allocation
   ▼
register bytecode (.klc)
   │
   ├──►  register VM (interpreter)
   └──►  Drop Bear JIT (x86-64 native, hot functions)
```

The JIT family is named after Australian fauna, following `koala` (the language) and `wombat` (the build tool): **Drop Bear** (v1 baseline) → **Yowie** (v2: unboxing + register allocation + direct calls) → **Bunyip** (v3: inline caches, inlining, AOT). See [`src/jit/DESIGN.md`](src/jit/DESIGN.md).

## Ecosystem

- **Standard library** (`std`): `io`, `fs`, `os`, `sys`, `time`, `socket`, `collections`, and the `ut` test framework.
- **Official libraries** (`koala`): `json`, `http`, `tls`, `redis`, `sqlite3`, `crypto`, `re`, `csv`, `datetime`, `gzip`, `zipfile`, `tarfile`, `base64`, `uuid`, `log`, `cli`, `openai`, `sse`, and more.
- **Native extensions**: a small C FFI lets you bind native libraries with `@native` / `@intrinsic`.

## Documentation

- [Koala Language Manual](docs/Koala_Language_Manual.md) — complete language reference
- [Design Overview](docs/Koala_Design_Overview.md) — design decisions and rationale
- [JIT Design (Drop Bear)](src/jit/DESIGN.md) — baseline JIT internals and roadmap
- [Standard Builtin API](docs/std_builtin_api.md) — generated `std/builtin` API reference

## Status & roadmap

Koala is young and under active development. The language core, optimizing compiler, baseline JIT and a broad standard/library ecosystem are working and tested. The **semi‑automatic memory management** model (deterministic `free` + a standby collector) is the announced design target and is **not yet enabled**.

On the roadmap:

- **Yowie** JIT v2 — unboxing, machine register allocation, JIT→JIT direct calls, inline float comparison
- Semi‑automatic memory management (compiler‑inserted `free`, escape analysis, standby CMS collector)
- Closure capture of locals and `self`
- **Bunyip** JIT v3 — inline caches, hot inlining, AOT compilation
- Concurrency API surface over the VM's green‑thread scheduler

## Contributing

Koala is small and hackable — contributions are welcome!
Feel free to open issues, submit PRs, or discuss design ideas.

## License

MIT — see [LICENSE](LICENSE). Koala is free to use, modify and distribute.