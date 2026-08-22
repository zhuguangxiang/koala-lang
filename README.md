# 🐨 Koala Programming Language

**Koala is a tiny, fast, modern, statically‑typed, register‑based language and VM with aggressive local optimizations.**

Koala is designed to be:

- **Simple** — Python‑like semantics, small syntax, no hidden machinery
- **Fast** — register VM, zero boxing, low dispatch overhead
- **Optimizable** — lightweight IR passes keep hot loops tight
- **Portable** — pure C/C++, zero dependencies
- **Hackable** — easy to read, easy to extend

## Overview

Koala compiles source code into a compact, register‑based bytecode executed by a lightweight VM written in C. Values live in registers with no boxing, and the interpreter runs bare — no JIT — yet already beats Lua across the board and most benchmarks against LuaJIT's interpreter core.

## 🛠 Installation & Build

Koala uses standard CMake and has **no external dependencies**.

### Clone

```bash
git clone https://github.com/zhuguangxiang/koala-lang.git
cd koala-lang
```

### Build

```bash
./build-release.sh
./build-debug.sh
```

This produces the `koala` executable.

### Run a program

```bash
./koala bench/sum.kl
```

## Language Examples

### Script

Top‑level statements run directly — no `main` required:

```kl
// hello.kl
for i in range(1, 6) {
    if (i % 2 == 0) { print(i) }
}
```

### Program

With a `main` function:

```kl
// sum.kl
func sum(n int) int {
    var sum = 0
    for i in range(1, n) {
        sum = sum + i
    }
    return sum
}

func main() {
    print(sum(100_000_000 + 1))
}
```

`range(a, b)` is exclusive (`[a, b)`). For the complete language specification see the [Koala Language Manual](docs/Koala_Language_Manual.md).

## Documentation

- [Koala Language Manual](docs/Koala_Language_Manual.md) — complete language reference
- [Standard Builtin API](docs/std_builtin_api.md) — generated `std/builtin` API reference

## Performance

**Koala vs Lua vs LuaJIT (JIT off)** — bare interpreter, hyperfine (mean, lower is better).

| Benchmark   | Koala     | Lua      | LuaJIT(-joff) | vs Lua | vs LuaJIT |
| ----------- | --------- | -------- | ------------- | ------ | --------- |
| sum         | **565.6ms** | 1074ms | 956.4ms       | 1.90×  | **1.69×** |
| loop-sum    | 270.5ms   | 298.2ms  | 247.7ms       | 1.10×  | 0.92×     |
| fib         | **2.927s**  | 4.208s | 3.242s        | 1.44×  | **1.11×** |
| ack         | 556.8ms   | 551.6ms  | 385.5ms       | ~1.00× | 0.69×     |
| tak         | 413.7ms   | 581.5ms  | 356.1ms       | 1.41×  | 0.86×     |
| shuffle     | **140.7ms** | 653.5ms | 461.6ms     | **4.64×** | **3.28×** |
| logic_chain | **1.114s**  | 1.857s | 1.222s        | 1.67×  | **1.10×** |
| float_arith | **254.5ms** | 539.5ms | 343.0ms     | **2.12×** | **1.35×** |

Static typing is free ammunition for the future JIT: type specialization, devirtualization (intf‑table slot inlining) and bounds‑check elimination need no guessing and no guards.

## Roadmap

- Global DSE (cross-basic-block)

- SSA construction

- GVN / CSE

- LICM (loop-invariant code motion)

- Inline

- JIT backend

- Standard library

- Garbage Collection

## Contributing

Koala is small and hackable — contributions are welcome!
Feel free to open issues, submit PRs, or discuss design ideas.
