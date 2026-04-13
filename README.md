# 🐨 Koala Programming Language

**A tiny, fast, modern, register‑based language & VM with a clean IR and aggressive local optimizations.**

Koala is a minimal but high‑performance programming language and register‑based virtual machine.

Koala is designed to be:

- **Simple** — small syntax, small VM, small IR
- **Fast** — register VM, low dispatch overhead
- **Optimizable** — IR designed for DSE, DCE, copy propagation
- **Portable** — pure C/C++, zero dependencies
- **Hackable** — easy to read, easy to extend

## Overview

Koala compiles source code into a compact, register‑based bytecode executed by a lightweight VM. The IR is intentionally minimal, making it easy to implement optimizations such as:

- **Single‑Basic‑Block Dead Store Elimination (DSE)**
- **Dead Code Elimination (DCE)**
- **Copy Propagation**
- **Constant Folding**

Koala’s VM is written in C and achieves performance close to LuaJIT’s interpreter mode.

## 🛠 Installation & Build

Koala uses standard CMake and has **no external dependencies**.

### Clone

bash

```bash
git clone https://github.com/zhuguangxiang/koala-lang.git
cd koala-lang
```

### Build

bash

```bash
./build-release.sh
./build-debug.sh
...
```

This produces the `koala` executable.

### Run a program

bash

```bash
./koala bench/sum.kl
```

## Language Example

go

```go
func sum(n int) int {
    var sum = 0
    for i in range(1, n) {
        sum = sum + i
    }
    return sum
}

print(sum(100_000_000 + 1))
```

`range(a, b)` is exclusive (`[a, b)`). The compiler lowers it to:

Code

```go
i = a
if i >= b goto end
cond:
do {
    ...
    i = i + 1
    if i < b goto cond
} while(1)
end:
```

This lowering is one reason Koala’s loop performance is extremely strong.

## Koala IR Example

Source:

go

```ir
var x = n + 1
x = n + 2
x = n + 3
return x
```

Lowered IR:

llvm

```ir
%0 = add %n, 1
move %x, %0

%1 = add %n, 2
move %x, %1

%2 = add %n, 3
move %x, %2

ret %x
```

After DSE + DCE:

llvm

```ir
%2 = add %n, 3
ret %2
```

## Optimization Pipeline

Koala performs a series of lightweight but highly effective optimizations on its IR. Because Koala’s IR is simple, register‑based, and non‑aliasing, these optimizations are both safe and fast.

### ✔ Constant Folding

Simplifies expressions like `1 + 2` or `n + 0` at compile time.

### ✔ Single‑BB Dead Store Elimination (DSE)

Removes redundant `move` instructions when a local variable is overwritten without being read.

1. Track the last write to each local.
2. Mark any write that is later read as **live**.
3. Delete all unmarked writes except the final one.
4. Run DCE to remove unused SSA values.

### ✔ Dead Code Elimination (DCE)

Removes unused SSA values after DSE or copy propagation.

### ✔ Constant & Copy Propagation

Replaces trivial copies (`move %a, %b`) with direct use of `%b`, reducing register pressure and instruction count.

### ✔ Global Propagation of `let` Variables

Koala’s `let` bindings are immutable by design. This allows the compiler to **globally propagate** their values across the entire function, not just within a single basic block.

For example:

go

```
let x = n + 1
let y = x * 2
return y + x
```

Lowered IR before propagation:

llvm

```
%0 = add %n, 1
move %x, %0

%1 = mul %x, 2
move %y, %1

%2 = add %y, %x
ret %2
```

After global propagation:

llvm

```
%0 = add %n, 1
%1 = mul %0, 2
%2 = add %1, %0
ret %2
```

This optimization:

- Eliminates unnecessary locals
- Enables further constant folding
- Reduces VM register usage
- Simplifies the IR for later passes

Because `let` variables never change and never alias, Koala can safely propagate them across all basic blocks without requiring SSA or global dataflow analysis.

### ✔ IR Simplification

A final cleanup pass that removes redundant instructions and normalizes IR patterns.

## Performance Comparison

**Koala vs Lua vs LuaJIT (JIT Off)**

All benchmarks run 10–12 times; mean ± standard deviation shown.

### 1. Sum Benchmark

| Language     | Mean Time             | Notes         |
| ------------ | --------------------- | ------------- |
| **Koala**    | **564.7 ms ± 4.9 ms** | bench/sum.kl  |
| Lua          | 1.045 s ± 0.005 s     | bench/sum.lua |
| LuaJIT -joff | 948.8 ms ± 3.0 ms     | bench/sum.lua |

### 2. Loop-Sum Benchmark

| anguage      | Mean Time             | Notes              |
| ------------ | --------------------- | ------------------ |
| **Koala**    | **277.8 ms ± 5.7 ms** | bench/loop-sum.kl  |
| Lua          | 294.6 ms ± 3.3 ms     | bench/loop-sum.lua |
| LuaJIT -joff | 245.5 ms ± 2.7 ms     | bench/loop-sum.lua |

### 3. Fibonacci Benchmark

| Language     | Mean Time             | Notes         |
| ------------ | --------------------- | ------------- |
| **Koala**    | **2.928 s ± 0.022 s** | bench/fib.kl  |
| Lua          | 4.135 s ± 0.050 s     | bench/fib.lua |
| LuaJIT -joff | 3.221 s ± 0.013 s     | bench/fib.lua |

### 4. Ackermann Benchmark

| Language     | Mean Time             | Notes         |
| ------------ | --------------------- | ------------- |
| **Koala**    | **562.6 ms ± 6.4 ms** | bench/ack.kl  |
| Lua          | 546.1 ms ± 5.4 ms     | bench/ack.lua |
| LuaJIT -joff | 383.5 ms ± 4.1 ms     | bench/ack.lua |

### 5. Tak Benchmark

| anguage      | Mean Time             | Notes         |
| ------------ | --------------------- | ------------- |
| **Koala**    | **421.8 ms ± 5.9 ms** | bench/tak.kl  |
| Lua          | 573.4 ms ± 10.0 ms    | bench/tak.lua |
| LuaJIT -joff | 353.2 ms ± 6.3 ms     | bench/tak.lua |

### 6. Shuffle Benchmark

| anguage      | Mean Time             | Notes             |
| ------------ | --------------------- | ----------------- |
| **Koala**    | **139.7 ms ± 3.7 ms** | bench/shuffle.kl  |
| Lua          | 602.7 ms ± 46.1 ms    | bench/shuffle.lua |
| LuaJIT -joff | 457.4 ms ± 16.3 ms    | bench/shuffle.lua |

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
