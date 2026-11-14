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
- **Drop Bear**, a baseline x86‑64 JIT (v1): hot integer/float/bool functions compile to native code, **up to ~9× over the interpreter** and already beats LuaJIT on several benchmarks.
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

**Koala vs LuaJIT vs Lua 5.4** — hyperfine (mean ms, lower is better), release build. `JIT` = Drop Bear baseline JIT (v1, with the write-back register cache); `int` = bare interpreter. Koala runs with `++ --recursive-self-inline --domtree --unroll`; regenerate the table with `./bench-table.sh`.

| Benchmark   | Koala JIT | Koala int | LuaJIT JIT | LuaJIT int | Lua 5.4 |
| ----------- | --------: | --------: | ---------: | ---------: | ------: |
| **fib** ⚡  | **274**   | 1256      | 694        | 3257       | 4128    |
| tak         | **137**   | 562       | 147        | 348        | 572     |
| loop-sum    | **31**    | 269       | 47         | 247        | 296     |
| float_arith | 52        | 268       | **47**     | 332        | 559     |
| logic_chain | 432       | 1260      | **343**    | 1237       | 1867    |
| float_logic | 81        | 713       | **65**     | 577        | 709     |
| sum         | 75        | 584       | **47**     | 957        | 1059    |
| ack         | 251       | 705       | **80**     | 381        | 539     |
| shuffle     | 71        | 351       | **20**     | 471        | 654     |

⚡ **fib** is run with `--recursive-self-inline` (the compiler inlines the two self-recursive
calls, then the JIT compiles the flattened body): **274 ms, 2.5× faster than LuaJIT**. Without
that flag fib is 797 ms (~0.86× of LuaJIT). The recursive-inline + JIT combo is where
static typing pays off most.

**loop-sum** is partially unrolled (`--unroll`, factor 4) and its loop body runs entirely in
machine registers thanks to the JIT's write-back register cache (values are written back to the
VM frame only at observable points such as calls and `ret`): **31 ms, 1.5× faster than LuaJIT**.

Bitwise benchmarks use Lua 5.4 integer syntax that LuaJIT (5.1-based) cannot parse, so they
compare against Lua 5.4 only — Koala JIT wins big: `test_bitwise` 138 ms vs 1150 ms (**8.3×**),
`test_arith` 483 ms vs 1752 ms (**3.6×**).

**Takeaways.** Drop Bear v1 speeds Koala up **3–9× over its own interpreter**. Against LuaJIT's
JIT it already wins on `tak`, `loop-sum` and `fib` (with recursive inlining), is close on
`float_arith`, and trails on call-heavy (`ack`, `shuffle`), tail-recursive (`sum`) and
`idiv`/branch-heavy (`logic_chain`) code — exactly what the next tier (Yowie: unboxing,
cross-call register allocation, cheaper calls) targets. Static typing is free ammunition: type
specialization, devirtualization and bounds-check elimination need no guessing and no guards.

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

[`test/test-bench/test_jit.kl`](test/test-bench/test_jit.kl) covers integer / uint / float loops and self‑ / tail‑recursion. Drop Bear v1 measured ~5.6× on integer/uint loops, ~4.5× on `fib`, ~9× on tail recursion and ~4.8× on float loops. `@bench` also compares compiler options, e.g. `++ --test --domtree --unroll`.

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