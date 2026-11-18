# Koala Concurrency Plan: Fibers, Scheduler, GC

> Status: **proposal**, 2026-11-17. Updated the same day: `wsdeque.c` review, `malloc` fix and
> multi-owner tests (§2.9), gray stack (§8.7), parallel-marking details (§9.9).
> Based on the code as of this date. Every file and function named here exists in the repo
> unless it is marked *(new)*.

---

## Contents

0. [Summary](#0-summary)
1. [Goals and non-goals](#1-goals-and-non-goals)
2. [What the current code gives us](#2-what-the-current-code-gives-us)
3. [Choosing the model](#3-choosing-the-model)
4. [Architecture overview](#4-architecture-overview)
5. [Phase 1: fibers and one KoalaState per coroutine](#5-phase-1-fibers-and-one-koalastate-per-coroutine)
6. [Phase 2: scheduler, tasks, channels, groups](#6-phase-2-scheduler-tasks-channels-groups)
7. [Phase 3: I/O integration and the blocking pool](#7-phase-3-io-integration-and-the-blocking-pool)
8. [Phase 4: stop-the-world GC with coroutines](#8-phase-4-stop-the-world-gc-with-coroutines)
9. [Phase 5: incremental SATB marking and background sweep](#9-phase-5-incremental-satb-marking-and-background-sweep)
10. [Phase 6: multi-core with isolates](#10-phase-6-multi-core-with-isolates)
11. [JIT impact, phase by phase](#11-jit-impact-phase-by-phase)
12. [Debugging and tooling](#12-debugging-and-tooling)
13. [Risks and mitigations](#13-risks-and-mitigations)
14. [Milestones and tests](#14-milestones-and-tests)
15. [Open questions](#15-open-questions)
16. [Appendix A: how other languages do it](#appendix-a-how-other-languages-do-it)
17. [Appendix B: glossary](#appendix-b-glossary)

---

## 0. Summary

| Decision | Choice |
|---|---|
| Unit of concurrency | **Stackful coroutine (fiber)**: one `KoalaState` plus one private stack region |
| Threads in v1 | **One** pthread runs all Koala code (N:1). A small pthread pool runs blocking C calls and never touches the GC heap |
| Syntax | **No new keywords.** No `async`, `await` or `yield`. Everything is library API in `std/task` |
| Suspension | Only inside native (C) functions: socket I/O, channel ops, sleep, join, blocking-pool calls |
| Structured concurrency | `with task.group() as g { ... }`, using the existing `__enter__`/`__exit__` protocol |
| Cancellation | A pending park returns "cancelled". The native raises a `Cancelled` error that unwinds through the existing error path |
| GC, v1 | Current stop-the-world mark & sweep. Roots come from **every** coroutine's VM stack |
| GC, v2 | **Incremental SATB marking** on the mutator thread (paced by allocation and run when the event loop is idle) plus **background sweep** on a helper thread |
| Multi-core | **Isolates**: one VM, heap, scheduler and event loop per pthread, with message passing. M:N on a shared heap is not planned |
| Compiler and JIT changes | **None** until the GC write barrier (Phase 5) |

---

## 1. Goals and non-goals

### Goals

1. Thousands to hundreds of thousands of concurrent network tasks in one process (HTTP, Redis, LLM
   clients, SSE, chat agents: the workloads already in `libs/koala` and `demo/`).
2. **Existing synchronous code keeps working and becomes concurrent without changes.** A function that calls
   `conn.read_line()` should not need rewriting to run inside a task.
3. No changes to the language grammar, the KLR IR, the optimizer passes or the `.klc` format.
4. No slowdown for single-threaded programs that never spawn a task (`bench/` must not regress).
5. Clear invariants for the GC and JIT, written down here, so later work (unboxed JIT, incremental
   GC, isolates) doesn't break concurrency.

### Non-goals (for now)

- Running Koala bytecode on several threads that share one heap.
- Preemptive scheduling (an optional time-slice check comes later, see §6.11).
- Async regular-file I/O (`io_uring` is a later option, see §7.6).
- A `select` statement in the language. A library `task.select` comes later, see §6.8.

---

## 2. What the current code gives us

These facts drive every decision below.

### 2.1 Interpreter: every Koala call recurses on the C stack

`OP_CALL` (`include/runtime/vm_ops.h`, `TARGET(OP_CALL)`) calls `kl_call_code`
(`src/runtime/eval.c`), which calls `_eval_frame`. So a chain of Koala calls is also a chain of
C frames. Suspending a coroutine means keeping its C frames: either the compiler rewrites
functions into state machines, or each coroutine gets its own C stack. This plan does the second.

### 2.2 `KoalaState` already has everything a coroutine needs

`include/runtime/vm.h`:

```c
typedef struct _KoalaState {
    CallFrame *cf;          int depth;
    int stack_size;         TValue *stack_base;
    TValue *stack_top;      TValue *stack_limit;   /* read by JIT */
    char *c_stack_limit;                            /* read by JIT */
    CallFrame *cf_cache;
    Object *builtin;        Object *sys;
    Object *exc;
    struct _ThreadState *ts;
} KoalaState;

typedef struct _ThreadState { KoalaState *current; } ThreadState;
extern __thread ThreadState *__ts;
```

The VM stack, frame chain, frame cache, pending exception and both stack limits are all per
`KoalaState`. `ThreadState.current` already means "the KoalaState running on this pthread".
Switching coroutines means switching `__ts->current` plus the machine context.

### 2.3 The JIT never reads thread-local state

The native signature is `void fn(TValue *regs, TValue *out, KoalaState *ks)` (`src/jit/DESIGN.md`
§4.1). Stack checks for JIT-to-JIT direct calls read `ks->stack_limit` and `ks->c_stack_limit`
(§5.3, step 2). Because `ks` is a parameter, **JIT code runs correctly on any coroutine's stack
as long as that coroutine's `ks` holds the right limits.**

### 2.4 The native stack holds no object references

JIT DESIGN §14.1, rule 5: every live value is in `cf->locals` on the VM stack. The native stack
(`rbp`/`rsp`, return addresses, scratch slots) holds no roots. So **the GC never scans a
coroutine's C stack**, which is usually the hard part of stackful coroutines in a garbage-collected
runtime.

### 2.5 GC safepoints are call sites

JIT DESIGN §14.1, rule 4: the GC runs only inside helper or runtime calls, where `ks->stack_top`
covers all live registers (self-only functions set it before each helper call in
`emit_call_abs`). **A coroutine can only suspend inside a native call, which is always a
safepoint.** So a suspended coroutine's VM stack can always be scanned with the same bound the GC
uses today.

### 2.6 GC: global state, single-threaded, non-moving, manual trigger

`src/runtime/gc.c`:

- All state is file-level `static`: `gc_normal_list`, `gc_perm_list`, `gc_root_list`,
  `gc_gray_dq`, counters, `gc_cur_state`.
- `gc_collect(KoalaState *ks)` scans roots from **one** `ks` (`gc_mark_roots`).
- Blocks are `calloc`'d (`GCHeader` + payload) and **never move**. A pointer to a `TValue` slot in a heap
  object stays valid.
- Collection runs only when Koala code calls `gc.collect()` (`src/modules/gc.c`).
- The state machine (`GC_MARK_ROOTS → GC_MARK → GC_SWEEP`) is the right shape for incremental
  marking later. The gray stack is the Chase-Lev deque `gc_gray_dq`, which no other thread ever
  steals from today; §8.7 replaces it with a plain stack until marking goes parallel (§9.9).
- Memory model (Design Overview §12): explicit `free`, compiler-inserted `free`, and `noesc` escape
  analysis are the main path; the collector is the safety net.

### 2.7 Known GC hazards that concurrency makes worse

From `koala_usage_issues.md`:

- **#11**: native code holds new objects in C locals, which are not roots. Today a GC can only
  happen at `gc.collect()`, so this rarely bites. Once another coroutine can run while a native is
  parked, it will.
- **#12**: VM stack slots above old frames can hold stale pointers that a later frame exposes to the
  GC. Fresh, zeroed stacks per coroutine help here (§5.5).

### 2.8 Parts already in the repo

| Part | Where | Use in this plan |
|---|---|---|
| Chase-Lev work-stealing deque, with a TSan test | `include/common/wsdeque.h`, `src/common/wsdeque.c`, `test/test-c/tsan_wsdeque.sh` | Reviewed and switched to `malloc` (§2.9). Used only for parallel marking (§9.9); the single-threaded GC gets a plain gray stack (§8.7) |
| Intrusive lists | `include/common/list.h` | Run queue, registry, wait lists (single thread, no locks) |
| Ring buffer | `include/common/ring.h` | Buffered channel storage |
| Per-`ks` shadow stack for native roots (`kl_gc_protect`) | `include/backup/shadowstack.h` | Bring back for issue #11 (§8.3) |
| Old multi-thread scheduler skeleton | `src/backup/run.c` | Reference only; v1 is single-threaded |
| Old concurrent GC (GC thread, STW through a guard-page trap, spinlocked allocation) | `src/backup/gc.c` | Reference only; Phase 5 uses a different design (§9.1) |
| `with` protocol | language manual, `test/test-kl/test_with_*.kl` | `task.group()`, `task.timeout()` |
| Non-blocking hints in `os` | `src/modules/os.c` | Pipes and `os.run` in Phase 3 |

### 2.9 `wsdeque.c`: review and status

`src/common/wsdeque.c` is the C11 Chase-Lev deque of Lê et al. (PPoPP 2013), the paper cited in
`wsdeque.h`. It matches the paper step for step:

| Operation | What it does | Why it is correct |
|---|---|---|
| `wsdq_push` | Store item (relaxed) → release fence → store `bottom` (relaxed) | A thief that reads `bottom` with acquire also sees the item |
| `wsdq_pop` | Store `bottom - 1` → **seq_cst fence** → read `top`; CAS on `top` only for the last item | The fence stops the owner and a thief from both taking the last item |
| `wsdq_steal` | Read `top` (acquire) → seq_cst fence → read `bottom` (acquire); read `array` with acquire; CAS on `top` | Pairs with the release store of a grown array in `ws_grow` |
| `ws_grow` | Copy into a doubled array, release-store it, **retire** (not free) the old one until `wsdq_fini` | A thief still reading the old array never touches freed memory |

Other good points: signed `int64_t` indices make the empty check (`t > b` with `b == -1`) correct
without unsigned wraparound; `top` and `bottom` sit on separate cache lines; the tests cover FIFO
steals, LIFO pops, growth from capacity 4, and 4 thieves against one owner under TSan
(`test/test-c/test_wsdeque.c`, `tsan_wsdeque.sh`).

Findings and status:

| # | Finding | Status |
|---|---|---|
| 1 | Arrays came from `mm_alloc`/`mm_free`. With `MM_TRACK_BLOCKS` (all debug builds) every block is linked into an **unlocked** global list (`src/common/mm.c`). One owner per deque is safe, which is why the test passed, but parallel markers each own a deque and can grow them at the same time, corrupting the list. A `size_t` byte count also overflowed through `mm_alloc`'s `int` parameter at about 2^28 slots | **Done (2026-11-17):** `ws_array_new` uses `calloc` with a `size_t` byte count and aborts on failure; `wsdq_fini` uses `free`. Tests pass normally and under TSan |
| 2 | Every `wsdq_pop` runs a seq_cst fence (`mfence` or a locked instruction on x86, tens of cycles). `gc_scan` pops once per gray object, but nothing steals from `gc_gray_dq`, so the fence buys nothing | Planned: plain gray stack for single-threaded marking (§8.7) |
| 3 | No test where **several owners grow their deques at the same time** (the situation finding 1 was about) | **Done (2026-11-17):** two tests added to `test_wsdeque.c` (below). They pass with the `malloc` version, and under `tsan_wsdeque.sh` they report a data race in `mm_alloc`'s block list when run against the old `mm_alloc` version |
| 4 | The deque never shrinks after a large collection | Accepted: fine for a GC mark stack |

Tests in `test/test-c/test_wsdeque.c` (9 in total):

| Test | What it checks |
|---|---|
| 1–6 | Empty deque, steal from empty, growth past capacity, FIFO steals, LIFO pops, pop after drain |
| 7 `stress` | One owner pushes 200,000 items with interleaved pops while 4 thieves steal; every item taken exactly once |
| 8 `multi_owner` *(new)* | 4 owners, each with its own deque starting at capacity 4, push 100,000 items each (popping every 7th push), so all 4 deques grow to 2^17 at the same time while 4 thieves steal from all deques in turn; every one of the 400,000 items taken exactly once, and every deque empty at the end. This is the parallel-marking shape (§9.9) |
| 9 `churn` *(new)* | 4 threads each run 200 rounds of init → push 1,024 → pop all (checking LIFO order) → fini on their own deque, so allocation and freeing happen concurrently from every thread |

Run `test/test-c/tsan_wsdeque.sh [rounds]` after any change to `wsdeque.c` or to how it
allocates memory.

Where it is used in this plan:

| Part of the plan | Use `wsdeque`? | Why |
|---|---|---|
| Phase 2 run queue | No | One thread; plain intrusive `List` |
| Phases 4–5 marking (stop-the-world, then incremental on the mutator) | No | Nothing steals; plain gray stack (§8.7) |
| Phase 5 parallel marking (§9.9) | **Yes** | One deque per marker thread; idle markers steal |
| Blocking-pool job queue (§7.6) | No | Many threads add jobs; Chase-Lev allows only one adder. Use a mutex and condition variable queue |
| Isolates (Phase 6) | No | Coroutines never move between isolates |
| M:N on a shared heap (not planned, §10.4) | Yes | Per-thread run queues with stealing, as in Go and Tokio |

---

## 3. Choosing the model

### 3.1 Options

| Option | What it means for Koala |
|---|---|
| A. pthreads sharing one heap | Make `gc.c`, the atom table, the module table, JIT counters and code arena, and every native library thread-safe. Add a multi-thread stop-the-world handshake. Define what happens on racy `list`/`dict` access. Years of work, with a single-thread cost (CPython free-threading: ~5–10% after several years). |
| B. Stackless coroutines (`async`/`await`) | Compiler turns async functions into state machines with heap frames. Needs new syntax, an IR pass, `.klc` changes and async versions of every I/O API. Can't suspend inside callbacks called from C (sort key, `__eq__`, iterator `next()`, `__exit__`). Async trait methods are hard. |
| C. **Stackful coroutines (fibers)** | Each coroutine gets its own C stack and VM stack. Suspension happens inside C functions. No compiler changes; existing sync APIs become non-blocking. Costs: per-architecture context switch, a few KB to tens of KB of memory per coroutine, and native code must root objects across parks. |

### 3.2 Why C

1. §2.3–§2.5 mean the JIT and GC already follow the rules fibers need.
2. Goal 2 (existing code becomes concurrent) is only possible with C.
3. Phases 1–4 need no compiler changes. The only compiler or JIT change in the whole plan is the
   Phase 5 write barrier, which any incremental GC needs anyway.
4. Other runtimes do the same: PHP 8.1 Fibers switch full stacks alongside PHP's JIT; Java
   virtual threads run JIT-compiled code; Ruby fibers; Zig 0.16 moved I/O behind an `Io`
   interface so the same code runs blocking or evented (Appendix A).

### 3.3 What C gives up, and how we answer it

| Cost | Answer |
|---|---|
| Suspension points aren't marked at call sites | Only natives can park. Mark them in docs and with a `@parks` doc tag on the `.kl` declaration (§6.12). A linter can list them. |
| Memory per coroutine | Lazy-committed `mmap`: actual memory is only pages touched (§5.4). |
| Context switch per architecture | ~30 lines of assembly each for x86-64 and arm64 (§5.2). The JIT is x86-64 only today. |
| Natives must root temporaries across parks | Per-`ks` shadow stack (§8.3) plus a GC stress mode (§12.4). |
| Native frames tie a coroutine to its thread | Irrelevant for N:1. With isolates (§10), coroutines never change threads. |

---

## 4. Architecture overview

```
 pthread (isolate) ─────────────────────────────────────────────────────────────
 │ ThreadState (__ts)
 │   current ──► KoalaState (running coroutine)
 │   sched   ──► Scheduler
 │                 runq:     [ks3] → [ks7] → ...        (intrusive list)
 │                 all:      registry of every live KoalaState (GC roots)
 │                 poller:   epoll / kqueue fd + per-fd wait records
 │                 timers:   min-heap of (deadline, wait record)
 │                 offload:  completion eventfd from the blocking pool
 │
 │ KoalaState (one per coroutine)
 │   VM stack, cf chain, cf_cache, exc, shadow stack, stack limits
 │   fiber ──► FiberCtx { saved rsp } + Region (mmap: C stack + VM stack)
 │   task  ──► TaskObject (GC object, visible to Koala code)
 │
 │ GC heap (gc.c): one per isolate, scanned from every ks in sched.all
 └──────────────────────────────────────────────────────────────────────────────
 blocking pool (pthreads) ── runs C jobs only, never touches Koala objects or mm_*
 GC sweeper thread (Phase 5) ── frees white blocks, never runs finalizers
```

Main-loop shape: there is **no separate scheduler fiber**. `kl_park()` picks the next runnable
coroutine and switches straight to it. If none is runnable, it polls for I/O and timers on the
current stack, then switches. One switch per handoff, not two.

---

## 5. Phase 1: fibers and one KoalaState per coroutine

### 5.1 New files

| File | Contents |
|---|---|
| `include/runtime/fiber.h` *(new)* | `FiberCtx`, `Region`, `kl_fiber_switch`, `kl_fiber_init`, region pool API |
| `src/runtime/fiber_x86_64.S` *(new)* | Context switch and entry trampoline, System V x86-64 |
| `src/runtime/fiber_arm64.S` *(new)* | Same for AArch64 (when the JIT or build needs arm64) |
| `src/runtime/fiber.c` *(new)* | Region allocation, pool, sanitizer and Valgrind annotations |

### 5.2 Context switch (x86-64)

Only callee-saved state is kept: `rbx`, `rbp`, `r12`–`r15`, `rsp`, and the MXCSR and x87 control words
(System V says these are preserved across calls). Caller-saved registers are already saved by the C
code that called `kl_fiber_switch`.

```asm
# void kl_fiber_switch(FiberCtx *from, FiberCtx *to)
#   rdi = from, rsi = to; FiberCtx's first field is `void *sp`
    .globl kl_fiber_switch
kl_fiber_switch:
    push    %rbp
    push    %rbx
    push    %r12
    push    %r13
    push    %r14
    push    %r15
    sub     $8, %rsp
    stmxcsr (%rsp)
    fnstcw  4(%rsp)
    mov     %rsp, (%rdi)        # from->sp = rsp
    mov     (%rsi), %rsp        # rsp = to->sp
    ldmxcsr (%rsp)
    fldcw   4(%rsp)
    add     $8, %rsp
    pop     %r15
    pop     %r14
    pop     %r13
    pop     %r12
    pop     %rbx
    pop     %rbp
    ret

# first entry of a new fiber: r12 = entry function, r13 = its argument
    .globl kl_fiber_trampoline
kl_fiber_trampoline:
    mov     %r13, %rdi
    and     $-16, %rsp          # SysV: rsp % 16 == 0 at the call
    call    *%r12
    ud2                         # entry functions never return (see §6.4)
```

`kl_fiber_init(ctx, region, fn, arg)` builds a frame that `kl_fiber_switch` "returns" into.
`top` is the 16-byte-aligned high end of the C-stack part:

| Address | Value | Popped into |
|---|---|---|
| `top - 16` | `kl_fiber_trampoline` | `ret` |
| `top - 24` | 0 | `rbp` |
| `top - 32` | 0 | `rbx` |
| `top - 40` | `fn` | `r12` |
| `top - 48` | `arg` | `r13` |
| `top - 56` | 0 | `r14` |
| `top - 64` | 0 | `r15` |
| `top - 72` | MXCSR `0x1F80`, x87 CW `0x037F` | `ldmxcsr` / `fldcw` |

`ctx->sp = top - 72`. After `ret`, `rsp = top - 8`, which is the alignment a called function expects.

Why not `ucontext`: `swapcontext` makes a `sigprocmask` system call on every switch. It's fine
for a first prototype, but don't ship it.

### 5.3 Region layout: one mapping per coroutine

Each coroutine gets **one** `mmap` region holding both stacks:

```
 low address                                                          high address
 ┌───────┬──────────────────────────────┬─────────────────────────────────────────┐
 │ guard │   C stack (grows down) ◄──── │ ────► VM TValue stack (grows up)         │
 │ page  │ c_stack_limit = lo+G+margin  │ stack_base = mid    stack_limit = hi     │
 └───────┴──────────────────────────────┴─────────────────────────────────────────┘
          ▲ lo + G                       ▲ mid (C stack top; trampoline frame here)
```

- **C stack at the low end, guard page below it.** A C helper that goes past `c_stack_limit`
  (for example deep recursion inside a native library) hits the guard page and faults, as a
  pthread's stack does today.
- **VM stack at the high end needs no guard.** Every growth is checked against `stack_limit`
  (`vm_frame_fits` in `kl_call_code` and the JIT direct-call check), so the VM stack never
  writes past `hi`.
- The C stack's top is `mid`, and `kl_fiber_init` puts the trampoline frame there.

**Why one mapping:** each `mprotect`'d guard page splits a mapping (VMA). Linux's default
`vm.max_map_count` is 65530. One region with one guard costs **2 VMAs per coroutine**, a
default cap of about 32,000 coroutines. Separate regions for the VM and C stacks would cost 3–4
VMAs and halve that. For more coroutines:

- raise `vm.max_map_count`, or
- set `KOALA_CO_GUARD=0`: no guard page, 1 VMA. Overflow is then caught only by
  `c_stack_limit` checks, which happen on every Koala call but not inside deep C helpers. Use the
  margin (§5.4) to cover the deepest C helper.

### 5.4 Sizes and limits

| Setting | Default | Env var | Notes |
|---|---|---|---|
| C stack per coroutine | 1 MB reserved | `KOALA_CO_CSTACK` | Virtual only; physical memory = pages touched |
| C-stack margin | 64 KB | `KOALA_CO_CMARGIN` | Kept free below `c_stack_limit` for C helpers. Today's `C_STACK_MARGIN` (256 KB, `src/runtime/vm.c`) stays for the main thread |
| VM stack per coroutine | 1 MB (64K slots) | `KOALA_CO_VMSTACK` | Main `ks` keeps `MAX_STACK_SIZE` |
| Guard page | on | `KOALA_CO_GUARD` | §5.3 |
| Pool size | 256 free regions | `KOALA_CO_POOL` | §5.5 |

Map with `mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE|MAP_STACK, -1, 0)`.
100k coroutines × 2 MB is 200 GB of *address space*: fine on 64-bit Linux with the default
overcommit setting, but it fails with `vm.overcommit_memory=2`. Document it, and keep the sizes
configurable.

**Measure before freezing the defaults.** The C stack used per interpreted Koala call
(`kl_call_code` + `_eval_frame` frame) sets how deep recursion can go inside a coroutine.
Measure it with `test/test-run/test_interpreter_stack.kl` and `test_jit_direct_call_deep.kl` run
inside a coroutine, then pick a C-stack default that allows at least the recursion depth the
VM stack allows.

### 5.5 Region pool and zeroed stacks

- Finished coroutines return their region to a free list (up to `KOALA_CO_POOL`). Beyond that,
  `munmap`.
- On return to the pool, `madvise(MADV_DONTNEED)` the VM-stack part and the touched C-stack part.
  Physical pages are released and **come back zero-filled**.
- `TAG_NIL == 0` and `nil_value` is all zeros (`include/runtime/object.h`), so a fresh or reused VM
  stack is all nil. This settles issue #12 for coroutine stacks: no stale pointers are ever
  visible to the GC.
- Do the same for the **main** `ks`: replace `aligned_alloc` + `memset` in `kl_new_ks` with an
  `mmap`. The 2 MB `memset` disappears from startup too.

### 5.6 Changes to `KoalaState`

```c
typedef struct _KoalaState {
    /* ... existing fields unchanged ... */

    /* (new) coroutine support */
    struct _FiberCtx  ctx;          /* saved rsp while not running */
    struct _Region   *region;       /* NULL for the main ks (runs on the pthread stack) */
    struct _TaskObject *task;       /* owning task, NULL for main */
    struct _Scheduler *sched;
    List   sched_link;              /* run queue or a wait list */
    List   all_link;                /* scheduler registry */
    uint32_t state;                 /* KS_RUNNABLE, KS_RUNNING, KS_PARKED, KS_DEAD */
    uint32_t cancel;                /* cancellation requested (§6.7) */
    uint32_t gc_epoch;              /* last GC cycle that scanned this stack (§9.4) */
    struct _ShadowStack *shadow;    /* native temporary roots (§8.3) */
    struct _WaitRecord *wait;       /* what this ks is parked on */
} KoalaState;
```

New constructor `KoalaState *kl_new_co_ks(Region *r)`:

- `stack_base = mid`, `stack_top = stack_base`, `stack_limit = hi`,
  `stack_size = (hi - mid) / sizeof(TValue)`
- `c_stack_limit = lo + guard + margin` (replaces `pthread_getattr_np` for coroutines)
- `builtin` and `sys` copied from the main `ks` (modules are shared)
- `ts = __ts`

`kl_free_ks` currently asserts `!ks->cf`. Add `kl_abandon_ks` for coroutines still parked at
program exit (§6.10): it frees the region and `cf_cache` without the assertion.

### 5.7 Sanitizers and Valgrind

Tests already use Valgrind and TSan (`test/test-c/tsan_wsdeque.sh`), so switching must be
annotated or those tools give false reports:

- ASan: `__sanitizer_start_switch_fiber` / `__sanitizer_finish_switch_fiber` around every switch.
- TSan: `__tsan_create_fiber`, `__tsan_switch_to_fiber`, `__tsan_destroy_fiber`.
- Valgrind: `VALGRIND_STACK_REGISTER(lo, hi)` when a region is created, `VALGRIND_STACK_DEREGISTER` when freed.

Wrap these in `fiber.c` behind `#ifdef` so release builds pay nothing.

### 5.8 Phase 1 deliverable

`kl_spawn_raw(fn, args, n)` runs a Koala function on a new fiber and switches back with a test-only
`kl_yield()`. This is enough for the M1 tests in §14 before the scheduler exists.

---

## 6. Phase 2: scheduler, tasks, channels, groups

### 6.1 Scheduler structure

`include/runtime/sched.h` *(new)*, `src/runtime/sched.c` *(new)*:

```c
typedef struct _Scheduler {
    KoalaState *main;        /* root coroutine: the pthread's original ks */
    List runq;               /* KoalaState.sched_link, FIFO */
    List all;                /* KoalaState.all_link, every live ks */
    int  nlive;
    Poller poller;           /* §7.1 */
    TimerHeap timers;        /* min-heap keyed by deadline */
    int  offload_fd;         /* eventfd: blocking-pool completions (§7.5) */
    List dead;               /* finished ks whose region can be recycled */
    uint64_t switches;       /* stats */
} Scheduler;
```

Single-threaded, so plain intrusive lists from `include/common/list.h`, not `lldq` (which takes a
spinlock on every operation).

### 6.2 Core C API

```c
/* make ks runnable; append to runq; never switches */
void kl_ready(KoalaState *ks);

/* suspend the current coroutine until someone calls kl_ready on it.
   returns KL_PARK_OK, KL_PARK_CANCELLED or KL_PARK_TIMEOUT. */
int  kl_park(WaitRecord *w);

/* create a coroutine running fn(args); runnable but not started */
KoalaState *kl_spawn(Object *fn, TValue *args, int nargs);

/* give up the CPU; stay runnable */
void kl_yield(void);
```

`kl_park` algorithm:

```
current.state = PARKED; current.wait = w
loop:
    if runq not empty:        next = pop(runq); break
    timeout = timers.empty ? -1 : max(0, timers.min - now)
    if nothing can ever wake anyone (no fds, no timers, no pending offloads):
        report deadlock (§12.2) and exit with error
    poller_wait(timeout)       # wakes coroutines with kl_ready
    fire expired timers        # kl_ready, with result TIMEOUT
switch_to(next)                # kl_fiber_switch(&cur->ctx, &next->ctx); __ts->current = next
on return: recycle regions in sched.dead; return current.wait->result
```

Switching rules:

1. `__ts->current` is updated **before** `kl_fiber_switch`, because code on the new stack reads
   `__ks()` straight away.
2. A switch happens only inside a native function: by §2.5 that's a safepoint, so the suspended
   `ks` has a valid `stack_top`.
3. **No one writes into a parked coroutine's VM stack.** Results go into its `WaitRecord`, and the
   coroutine copies them into its own registers after waking. Phase 5 depends on this rule (§9.4).

### 6.3 Wait records

```c
typedef struct _WaitRecord {
    KoalaState *ks;
    int kind;           /* WAIT_FD, WAIT_TIMER, WAIT_CHAN_SEND, WAIT_CHAN_RECV, WAIT_JOIN, WAIT_OFFLOAD */
    int result;         /* KL_PARK_* */
    TValue value;       /* value handed over (channel recv, join) */
    List link;          /* on the object being waited for */
    TimerNode timer;    /* optional deadline */
} WaitRecord;
```

A `WaitRecord` lives in the parked native's C frame (on the parked coroutine's C stack), so it
needs no allocation. Its `value` field holds a reference **that the GC must see**: the wait
record is reachable through `ks->wait`, and `gc_mark_roots` marks `ks->wait->value` (§8.1).

### 6.4 Coroutine lifetime

```
kl_spawn ──► RUNNABLE ──(picked)──► RUNNING ──(kl_park)──► PARKED
                ▲                      │                       │
                └──────(kl_ready)──────┴───────────────────────┘
RUNNING ──(entry function returns)──► DEAD ──(region recycled by the next park)──► freed
```

The fiber entry function `kl_co_main(KoalaState *ks)`:

1. Calls `kl_call_code(task->fn, task->args, n)`.
2. Stores the result in the `TaskObject`. If it's `error_value`, moves the exception from `ks->exc`
   into `task->exc`.
3. Wakes every joiner (`kl_ready`).
4. Sets `state = DEAD`, moves `ks` from `sched.all` to `sched.dead`.
5. Switches away and never returns. A coroutine can't free the stack it's running on, so the next
   `kl_park` on another coroutine recycles the region.

### 6.5 `TaskObject` (GC object)

```c
typedef struct _TaskObject {
    OBJECT_HEAD
    KoalaState *ks;        /* NULL once dead and recycled */
    int state;             /* PENDING, RUNNING, DONE, FAILED, CANCELLED */
    Object *fn;            /* until started */
    TValue *args; int nargs;
    TValue result;
    Object *exc;           /* when FAILED */
    List joiners;          /* WaitRecord.link */
    struct _GroupObject *group;
    uint8_t joined;        /* someone saw the result or error */
} TaskObject;
```

`_mark` marks `fn`, `args`, `result`, `exc`, `group`. While the coroutine is alive, its `ks` (in
`sched.all`) marks `ks->task`, so a running detached task is never collected.

### 6.6 Koala API: `libs/std/task` *(new)*

Readable `.kl` declarations over thin `@native` functions, following the same convention as the
rest of `libs/std`.

```kl
// task.kl (outline)
pub class Task[T] {
    pub func join() T            // parks until done; re-raises the child's error
    pub func cancel()
    pub func done() bool
}

pub func spawn[T](fn func(...) T, args ...any) Task[T]   // final generic form TBD by the type checker
pub func sleep(d time.Duration)
pub func yield_now()
pub func current() Task[any]?

pub class Channel[T] {
    pub func __init__(cap int = 0)
    pub func send(v T)           // parks while full; error if closed
    pub func recv() T?           // parks while empty; nil when closed and drained
    pub func try_send(v T) bool
    pub func try_recv() T?
    pub func close()
    pub func __iter__() ...      // for v in ch { ... } until closed
}

pub class Group {               // returned by task.group()
    pub func spawn[T](fn func(...) T, args ...any) Task[T]
    pub func __enter__() Group
    pub func __exit__(...)       // join all; on a child error cancel the rest, then re-raise the first
}

pub func group() Group
pub func timeout(d time.Duration) Group   // a Group that cancels its children at the deadline
```

Example:

```kl
import task
import socket

func handle(c socket.Conn) {
    while true {
        let line = c.read_line()     // parks this coroutine only
        if line == nil { break }
        c.write(line!)
    }
}

func main() {
    let ln = socket.listen(":8080")
    with task.group() as g {
        while true {
            g.spawn(handle, ln.accept())
        }
    }
}
```

`spawn` makes the task runnable and returns **without switching**: the caller keeps running until
it parks. This makes ordering deterministic and spawning cheap.

### 6.7 Cancellation

- `task.cancel()` sets `ks->cancel = 1`. If the task is parked, it is removed from whatever it
  waits on (fd, timer, channel queue, join list) and made runnable with
  `result = KL_PARK_CANCELLED`.
- The native that parked sees `KL_PARK_CANCELLED`, raises a `Cancelled` exception through the
  existing `raise_exc_*` helpers and returns `error_value`. Interpreter (`if (is_error(&ret)) goto error;`)
  and JIT (`cmp rax, TAG_ERROR`) both propagate it. Nothing new in the compiler or JIT.
- **Edge-triggered:** cancellation is delivered once. Later parks in the same coroutine (for
  example an `__exit__` that closes a connection gracefully) work normally. A group with a deadline
  enforces an upper bound by cancelling again at the deadline.
- If the task isn't parked, cancellation is delivered at its next park. CPU-bound code with no
  parks can't be cancelled until §6.11 exists.
- Cancelling a task that hasn't started marks it `CANCELLED` and never runs it.

### 6.8 Channels

- Storage: `ring.h` for `cap > 0`. `cap == 0` is a rendezvous: `send` hands the value straight to a
  waiting receiver's `WaitRecord.value`.
- Wait queues: `sendq` and `recvq` lists of `WaitRecord`, FIFO, so waiting is fair.
- `close()` wakes all receivers with `nil` and all senders with an error.
- `_mark` marks buffered values and the `value` of every queued sender's wait record.
- Later: `task.select([ch1.recv_op(), ch2.send_op(v), task.after(d)])` returns the index of the
  operation that completed. It's a library function built on the same wait records, with no new syntax.

### 6.9 Groups (structured concurrency)

- `Group.__exit__` parks until every child is done.
- If a child fails: cancel all siblings, wait for them, then re-raise the **first** error in the
  parent (later errors are attached to it as suppressed errors, for the traceback).
- If the parent itself is cancelled while waiting in `__exit__`, the group cancels its children and
  still waits for them. A group never leaks running children.
- `task.timeout(d)` is a group plus a timer that cancels the group at the deadline. `__exit__`
  then raises `Timeout` instead of `Cancelled`.

### 6.10 Program start and exit

- `koala_initialize` (`src/runtime/vm.c`) creates the `Scheduler` and registers the main `ks` as the
  root coroutine. A program that never spawns pays only that setup.
- `kl_run_main` runs `main` as today. When `main` returns, **the program ends** (as in Go). Use a
  group if children must finish first.
- At exit, coroutines still parked are abandoned without running (`kl_abandon_ks`), then
  `koala_finalize` runs as now.
- Errors: a task that fails and is never joined (no group, nobody called `join`) prints its traceback
  when its `TaskObject` is freed or at program exit, like an uncaught error in `main`
  (`print_exc_and_free`).

### 6.11 Fairness and optional preemption (later)

v1 is cooperative: a coroutine runs until it parks. A CPU-bound loop starves the others. Add
`task.yield_now()` for v1, and note the limit in the docs.

Later option, with no cost for code that doesn't need it:

- A timer signal (`setitimer`, e.g. every 10 ms) sets a `volatile` flag `ks->preempt`.
- The interpreter checks it at back-edges and calls. The JIT checks it where the planned OSR
  back-edge counter (JIT DESIGN §17) already decrements, so there's no extra branch in loops.
- When set, the check calls `kl_yield()`. Back-edges and calls are safepoints, so this is safe for
  the GC.

### 6.12 Marking natives that park

Every `@native` function that can park gets a doc tag in its `.kl` declaration:

```kl
/// Reads one line. @parks
@native
pub func read_line() str?
```

`tools/kl-doc.py` and `tools/kl-html-doc.py` show it, and a linter can list every parking
call in a function. This gives readers the "where can this suspend?" information that `await`
would, without colored function types.

---

## 7. Phase 3: I/O integration and the blocking pool

### 7.1 Poller

`src/runtime/poller_epoll.c`, `src/runtime/poller_kqueue.c` *(new)*, behind one interface:

```c
int  poller_init(Poller *p);
int  poller_wait_fd(int fd, int events, int64_t deadline_ns);   /* parks; returns KL_PARK_* */
void poller_poll(Poller *p, int timeout_ms);                    /* called from kl_park */
void poller_forget(int fd);                                     /* before close() */
```

- Use one-shot registration (`EPOLLONESHOT`, or `EV_ONESHOT` with kqueue) so a stale registration
  never wakes the wrong coroutine.
- A per-fd table maps fd → waiting `WaitRecord` for read and write (at most one reader and one writer
  per fd; a second concurrent reader gets an error, as in Go).
- `close()` on a socket calls `poller_forget` and wakes any waiter with an error.

### 7.2 One rule for all I/O natives

```c
/* the pattern every socket native follows */
for (;;) {
    ssize_t n = recv(fd, buf, len, flags);
    if (n >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) return n;
    int r = poller_wait_fd(fd, POLL_IN, deadline);
    if (r != KL_PARK_OK) return r == KL_PARK_TIMEOUT ? raise_timeout() : raise_cancelled();
}
```

When no other coroutine exists, `poller_wait_fd` just waits in `epoll_wait`, which behaves like
today's blocking call. **One code path serves both single-task and concurrent programs.**

The `buf` here is a Koala `bytes` object's data. It stays valid across the park because the
object is held by the native's arguments on the VM stack (a root) and the GC never moves objects.

### 7.3 `src/modules/socket.c`

| Function | Change |
|---|---|
| socket creation | Always `O_NONBLOCK` and `FD_CLOEXEC` |
| `_sys_connect` | `EINPROGRESS` → wait writable → `getsockopt(SO_ERROR)` |
| `_sys_accept` | `accept4(..., SOCK_NONBLOCK \| SOCK_CLOEXEC)`; on `EAGAIN` wait readable |
| `_sys_send`, `_sys_recv` | §7.2 pattern; partial writes loop at the `.kl` level as today |
| timeouts | `Conn.set_deadline(t)` stores a deadline used by every wait on that socket |

Everything above `socket` (`libs/koala/http`, `redis`, `openai`, `sse`) becomes concurrent with no
changes.

### 7.4 TLS (`libs/koala/tls`)

Use OpenSSL with the socket in non-blocking mode. On `SSL_ERROR_WANT_READ` or
`SSL_ERROR_WANT_WRITE`, wait for that event and retry the same `SSL_*` call. The handshake works the
same way.

### 7.5 Pipes, processes, signals, timers, stdin

- **Pipes from `os.pipe()` and `os.run()`**: created with `O_NONBLOCK` and go through the poller.
  `Process.wait()` parks on a `pidfd` (Linux 5.3+) or a `SIGCHLD` signalfd.
- **Signals**: a `signalfd` (Linux) or `EVFILT_SIGNAL` (kqueue) registered once. `os.signal`
  handlers run as tasks.
- **`time.sleep`**: parks on a timer. Same path for main and spawned tasks.
- **Inherited fds (0, 1, 2 and fds passed in)**: **never** set `O_NONBLOCK`. It changes the open file
  description, which is shared with the parent shell. Instead wait for readiness with the poller,
  then do a normal blocking `read`, which won't block for ttys and pipes once they're readable.
  Writes to stdout and stderr stay blocking.

### 7.6 Calls that must block: the offload pool

Some C calls can't be made non-blocking: `getaddrinfo`, sqlite, regular-file I/O, readline's
terminal handling, some native libraries.

C API (not exposed to Koala code: the pool can't run Koala functions):

```c
/* run job(ctx) on a pool thread; park until it finishes */
int kl_run_blocking(void (*job)(void *ctx), void *ctx);
```

- Pool: 4 pthreads by default (`KOALA_BLOCKING_THREADS`), started on first use.
- Job queue: a mutex and condition variable around an intrusive list. Not `wsdeque`: jobs can
  be added from several threads (one mutator per isolate in Phase 6), and Chase-Lev allows only
  one thread to add (§2.9).
- On completion the worker writes to the scheduler's `eventfd`; the poller wakes the waiting
  coroutine.
- **Rule: pool threads never touch the GC heap, `TValue`s, `mm_*` or any Koala object.** The native copies
  its inputs into a C struct (`ctx`) before parking and builds Koala objects after waking.
  `mm.c` tracks blocks in debug builds (`MM_TRACK_BLOCKS`) and isn't thread-safe.
- A C handle owned by a Koala object (e.g. `sqlite3 *` inside a connection object) may be used by the
  worker, because the parked coroutine keeps the object alive on its stack. Two coroutines using
  the same handle at once must be serialized: give such objects a coroutine mutex (a wait list, no
  pthread lock).
- Cancellation: the coroutine stops waiting (`KL_PARK_CANCELLED`), but the job runs to completion on
  the worker, and its result is dropped. The `ctx` must then be owned by the job, not the C stack.

Uses:

| Library | Use |
|---|---|
| DNS (`getaddrinfo`) | Offload |
| `libs/koala/sqlite3` | Offload each `step`/`exec`; per-connection coroutine mutex |
| Regular files (`src/modules/fs.c`) | Small reads and writes stay blocking (fast); `read_all` of large files offloaded. `io_uring` later |
| `libs/koala/readline` | Prefer the callback API (`rl_callback_read_char`) driven by stdin readiness; offload only if that's not possible |

---

## 8. Phase 4: stop-the-world GC with coroutines

### 8.1 Roots from every coroutine

Change `size_t gc_collect(KoalaState *ks)` to `size_t gc_collect(void)`. The `gc.collect()` native
in `src/modules/gc.c` keeps working. `gc_mark_roots` becomes:

```c
static void gc_mark_roots(void)
{
    /* perm root objects: unchanged */
    list_foreach(hdr, link, &gc_root_list) { ... tp->_mark(obj) ... }

    Scheduler *s = __ts->sched;
    gc_mark(s->main->builtin);
    gc_mark(s->main->sys);

    KoalaState *ks;
    list_foreach(ks, all_link, &s->all) gc_mark_ks(ks);
}

static void gc_mark_ks(KoalaState *ks)
{
    for (CallFrame *cf = ks->cf; cf; cf = cf->back) {
        gc_mark(cf->code);
        gc_mark(cf->module);
    }

    /* same bound as today (JIT DESIGN §14.1 rule 2), valid for parked ks by §2.5 */
    TValue *end = ks->stack_top;
    if (ks->cf && ks->cf->code) end += ks->cf->code->cs.max_call_args;
    for (TValue *v = ks->stack_base; v < end; v++) gc_mark_value(v);

    gc_mark(ks->task);
    if (ks->exc) gc_mark(ks->exc);
    if (ks->wait) gc_mark_value(&ks->wait->value);
    gc_mark_shadow(ks->shadow);           /* §8.3 */
}
```

Dead coroutines waiting for region recycling are removed from `all`, so they aren't scanned.

### 8.2 Invariants (must hold, test in debug builds)

1. **A coroutine switches only inside a native function** (§6.2, rule 2). Assert in
   `kl_fiber_switch`'s C wrapper that the caller is a C function (for example a per-`ks`
   `in_native` depth counter > 0).
2. **For every parked `ks`, `stack_top` covers all live registers.** Holds by JIT rule 4,
   because parks happen inside helper calls.
3. **Nobody writes a parked coroutine's VM stack** (§6.2, rule 3).
4. **No reference to a Koala object is held only in a C local across a park or an allocation**,
   unless it's protected (§8.3).

### 8.3 Native temporary roots (issue #11)

Bring back the shadow stack from `include/backup/shadowstack.h`. It already pushes onto a per-`ks`
list (`__ks()->shadow_stacks`), which is what coroutines need:

```c
static TValue _os_pipe(TValue *self, TValue *args, int nargs)
{
    Object *r = kl_new_raw_reader(...);
    Object *w = NULL;
    kl_gc_protect(r, w);          /* both scanned until this C scope ends */
    w = kl_new_raw_writer(...);   /* may allocate, park or call back into Koala */
    return obj_value(kl_new_tuple2(r, w));
}
```

Differences from the backup version:

- Store the slot address (`void **`), not the value, so later assignments (`w = ...`) are seen.
- Drop the `log_info` on every push and pop (or keep it at `trace` level).
- Field name `ks->shadow` (§5.6).

Audit order, from the native functions most likely to park or allocate in between:
`src/modules/os.c` (`run`, `pipe`), `socket.c`, `dictobj.c` and `hashset.c` paths that call
`__hash__`/`__eq__`, and `libs/koala/*` natives that build several objects.

### 8.4 When the GC runs

Two steps, to keep issue #11 from becoming a crash:

- **4a: trigger at park points only.** In `kl_park`, before switching: if
  `gc_get_count()` or `gc_used()` is over a threshold, `gc_collect()`. Natives that park must already
  follow invariant 4, and the stress mode `KOALA_GC_STRESS=park` (§12.4) collects on **every** park
  to find violations.
- **4b: trigger on allocation** (`gc_alloc_block`) once the §8.3 audit is done.
  `KOALA_GC_STRESS=alloc` collects on every Nth allocation.

Threshold: start with Lua-style pacing, collecting when `gc_used()` reaches twice the live size after
the previous collection, with a minimum (e.g. 4 MB). Make it configurable (`KOALA_GC_PAUSE`).

### 8.5 Explicit `free` and escape analysis

Stackful coroutines don't transform function bodies, so `free`, compiler-inserted `free` and
`noesc` keep working as before. New rules for the escape analysis in `src/parser/opt/`:

- Arguments to `task.spawn`, `Group.spawn`, `Channel.send`, `try_send`, and values captured by a
  lambda passed to `spawn`, **always escape**. The compiler never auto-frees them.
- An explicit `free(x)` of an object another task can still see is a use-after-free, as it is
  today with aliases in one task. Concurrency doesn't create a new kind of bug, but makes it easier
  to hit. Document it, and in debug builds poison freed blocks so such reads fail quickly.

### 8.6 Finalizers

`_free` finalizers (`raw_free`, `process_free`, file close) run during sweep on the mutator thread,
as today. In Phase 5 they move to a queue (§9.7).

### 8.7 Gray stack

`gc_gray_dq` is a `WSDeque`, but marking is single-threaded until §9.9, so the seq_cst fence in
every `wsdq_pop` is pure cost (§2.9, finding 2). Replace it with a plain growable array:

```c
/* include/runtime/gc.h or private to gc.c (new) */
typedef struct _GrayStack {
    void **items;
    size_t top;
    size_t cap;
} GrayStack;

static inline void gray_push(GrayStack *s, void *p)
{
    if (__builtin_expect(s->top == s->cap, 0)) gray_grow(s);   /* realloc to 2 * cap */
    s->items[s->top++] = p;
}

static inline void *gray_pop(GrayStack *s)
{
    return s->top ? s->items[--s->top] : NULL;
}
```

- Allocate with `malloc`/`realloc`, not `mm_*` (same reason as §2.9, finding 1, once a sweeper
  thread exists).
- LIFO order keeps marking depth-first, as today.
- Keep a high-water mark; after a cycle whose peak was far below `cap`, shrink back to a default
  (e.g. 1024) so one huge collection doesn't pin memory.
- `gc_mark` (push) and `gc_scan` (pop) are the only users. Phase 5's incremental slices pop a
  bounded number of entries per slice.
- **Measure** before and after: `GCCycleStats.elapsed_ms` on a large heap (for example a
  `test_gc_*.kl` with a few million live objects). Expect a gain because every pop currently runs
  a full fence; keep the change only if the numbers agree.
- Parallel marking (§9.9) swaps it back for one `WSDeque` per marker.

---

## 9. Phase 5: incremental SATB marking and background sweep

### 9.1 Why not the old concurrent design (`src/backup/gc.c`)

The backup GC marks on a separate pthread while the mutator runs. With Koala's value
representation that has a real hazard: **torn reads of 16-byte `TValue`s.** A heap slot of
interface or optional type can change between a scalar and a reference
(`{tag, payload}`, see `gc_mark_value`'s `is_intf` branch). JIT inline stores and C struct
assignments write the two 8-byte halves separately. A marking thread can then see a reference tag
with a scalar payload and follow a garbage pointer.

Making every heap `TValue` store atomic is possible (16-byte aligned `MOVDQA`/`VMOVDQA` are
atomic on CPUs that support AVX; `STP` is atomic with ARMv8.4 LSE2), but every store site in C, the
interpreter and the JIT would have to follow it. That's a large audit, for little gain while
there's only one mutator thread.

**So mark on the mutator thread, incrementally, and run only the sweep on another thread.** The
sweep reads only block headers of unreachable blocks, so it can't tear.

### 9.2 States

```
GC_DONE
  └─► GC_SNAPSHOT     atomic step (short): roots of running ks, globals, perm roots, builtin/sys
        └─► GC_MARK    incremental slices on the mutator thread; parked stacks scanned lazily
              └─► GC_REMARK   atomic step: drain gray stack, scan stacks not yet scanned
                    └─► GC_SWEEP   background thread frees white blocks
                          └─► GC_DONE
```

This extends the existing `GCState` in `src/runtime/gc.c`.

### 9.3 SATB write barrier

SATB (snapshot-at-the-beginning, a Yuasa deletion barrier) guarantees: **everything reachable at the
start of the cycle, plus everything allocated during it, survives.** When the mutator overwrites a
reference in the heap, the old value is shaded first:

```c
extern uint8_t gc_marking;          /* 1 during GC_SNAPSHOT..GC_REMARK */

static inline void gc_write(TValue *slot, TValue val)
{
    if (__builtin_expect(gc_marking, 0)) gc_mark_value(slot);   /* shade old value */
    *slot = val;
}

static inline void gc_write_obj(Object **slot, Object *val)
{
    if (__builtin_expect(gc_marking, 0)) gc_mark(*slot);
    *slot = val;
}
```

Because marking runs on the same thread, the barrier can gray the old value directly. No
SATB buffers, no atomics.

**Stores that need no barrier:**
- Writes to VM-stack registers: the interpreter's register moves, the JIT's write-back register
  cache (DESIGN §15), call arguments. Stacks are roots, and SATB doesn't need barriers on roots.
- `OP_GLOBAL_SET`: module value arrays are scanned in the atomic `GC_SNAPSHOT` step (§9.4), so they
  count as roots.
- Initializing stores into objects allocated during the cycle (they're allocated black, §9.5).
  Leaving the barrier in is harmless; leaving it out is an optimization.

**Stores that need a barrier** (the audit list):

| Where | Sites |
|---|---|
| Interpreter | `OP_SET_FIELD` (`vm_ops.h`) |
| Interpreter → C setters | `OP_SEQ_SET`, `OP_SEQ_SET_IMM`, `OP_SEQ_SET_SLICE` reach slot functions in `listobj.c`, `dictobj.c`, `hashset.c`, `src/objects/collections/*` |
| C objects | Every store of a `TValue` or `Object *` into an existing heap object: list grow/insert/remove (shifting slots overwrites references), dict insert/delete/resize, exception fields (`excobj.c`), function-object captures (`funcobj.c`), module objects (`modobj.c`) after load, `bytebuf.c` (only if it holds references) |
| Native libraries | `libs/std/*` and `libs/koala/*` natives that store into Koala objects |
| JIT | Inline `SET_FIELD` (DESIGN §5.7): emit `cmp byte [gc_marking], 0; jne .slow` before the store, where `.slow` calls `gc_mark_value(slot)` and jumps back |

Find the C sites with a grep for stores to `->values[`, `fields[`, `->items`, `Storage.values`, and
`TValue *` assignments in `src/objects/` and `src/modules/`. Add a debug check: in `GC_MARK`, a
verifier pass at `GC_REMARK` re-traverses from the roots and asserts every reachable block is black
(`KOALA_GC_VERIFY=1`). A missing barrier shows up as a white reachable block.

Cost when not marking: one load of a global byte and a predicted branch per heap reference store.

### 9.4 Snapshot and lazy stack scanning

The atomic `GC_SNAPSHOT` step scans:
- `gc_root_list` (perm root objects' `_mark`, including module globals)
- `builtin`, `sys`
- **only the running coroutine's** stack (`gc_mark_ks(current)`), then sets
  `current->gc_epoch = gc_cycle`

Parked coroutines are scanned **later but before they can change**:

- `kl_park`, just before switching to `next`: if `gc_marking && next->gc_epoch != gc_cycle`,
  call `gc_mark_ks(next)` and set its epoch.
- Mark slices also scan some parked stacks that haven't been scanned yet, so `GC_REMARK` has
  little left to do.
- `GC_REMARK` scans every `ks` still not at the current epoch.

**Why this is correct:** SATB needs the roots exactly as they were at the snapshot. A parked
coroutine's stack doesn't change until the coroutine runs again (rule 3 in §6.2: nobody else
writes into it). So scanning it any time before it resumes gives the same result as scanning it at
the snapshot. New coroutines spawned during the cycle start with `gc_epoch = gc_cycle` (their stack
starts empty; the function and arguments live in the black `TaskObject`).

**Pause:** the atomic step costs one stack plus the roots, no matter how many coroutines are parked.

### 9.5 Allocation during a cycle

- During `GC_SNAPSHOT..GC_REMARK`: new blocks are allocated **black** onto `gc_normal_list`
  (marking is on the mutator thread, so no locking is needed).
- During `GC_SWEEP`: the sweeper owns `gc_normal_list`. New blocks go to a separate
  `gc_new_list` (white, age 0), appended to `gc_normal_list` when the sweep finishes.
- `gc_used_size` and `gc_count` become `_Atomic`, because the sweeper subtracts from them.
- Remove `ASSERT(gc_cur_state == GC_DONE)` from `gc_alloc_block`.

### 9.6 Explicit `free` during a cycle

`gc_free` asserts `GC_DONE` today. During a cycle:

- Set a `GC_FLAG_PENDING_FREE` bit in the header and push the block on a pending list. Don't release it:
  it may be on the gray stack, or the sweeper may be reading its header.
- Marking treats pending blocks as black and doesn't scan them (the program said they're dead,
  and scanning would follow pointers that may also be freed).
- The sweeper skips pending blocks.
- After the cycle, the mutator releases the pending list.

### 9.7 Background sweep and finalizers

- A sweeper pthread (started with the first cycle) walks `gc_normal_list`:
  - black → white, `age++` (as today)
  - white without `_free` → `free()` (libc `free` is thread-safe)
  - white with `_free` → move to a **finalize queue**, not freed yet
- The mutator drains the finalize queue in `kl_park` when idle (before polling with a non-zero
  timeout) or at the next cycle start: run `_free`, then release the block.
- Finalizers therefore always run on the mutator thread, where they can safely use `mm_*`, close
  fds and touch other state.
- `gc_hooks.on_free` (debug output in `src/modules/gc.c`) is also called from the queue, on the
  mutator thread.

### 9.8 Pacing

Lua-style:

- **Allocation-paced slices:** after every `KOALA_GC_STEP` bytes allocated (default 64 KB), do
  `KOALA_GC_STEPMUL`% of that much marking work (default 200%). Run at the next safepoint, not
  inside `gc_alloc_block`, until §8.3's audit is done.
- **Idle slices:** in `kl_park`, if no coroutine is runnable and the next timer isn't due, mark
  for up to ~1 ms before polling. Event-driven programs then mostly collect while waiting for I/O.
- Start a cycle when `gc_used()` reaches `KOALA_GC_PAUSE`% of the live size after the last cycle
  (default 200%).

### 9.9 Later: parallel marking

If marking becomes the bottleneck on many-core machines:

- Make heap `TValue` stores 16-byte atomic during marking (§9.1).
- Replace the gray stack (§8.7) with one `WSDeque` per marker thread. Each marker pushes and pops
  its own deque; idle markers call `wsdq_steal` on the others (retry on `WS_ABORT`, try the next
  victim on `WS_EMPTY`). The `malloc` fix and the multi-owner tests (§2.9, findings 1 and 3)
  are already in; `multi_owner` in `test_wsdeque.c` exercises this exact pattern.
- Termination: a marker that finds every deque empty decrements an atomic "active markers"
  count and keeps checking; marking ends when the count reaches zero and all deques are empty.
- Change the barrier to push into per-thread SATB buffers.

Not needed for the first incremental GC.

---

## 10. Phase 6: multi-core with isolates

### 10.1 Model

- `isolate.spawn("worker.kl", args...)` starts a pthread that runs its own VM: its own GC heap,
  scheduler, event loop and module instances.
- Isolates share **no mutable Koala objects**. They talk over cross-isolate channels that carry
  deep copies of values, or immutable frozen objects.
- Inside one isolate everything in Phases 1–5 applies unchanged.

### 10.2 Refactor: move globals into per-isolate structs

| State | Today | Isolate design |
|---|---|---|
| GC lists, deque, counters, state | `static` in `src/runtime/gc.c` | `GCHeap` struct, reached through `__ts->heap` |
| Scheduler, poller, timers | *(new)* | `__ts->sched` |
| Module table | `kl_init_module_stbl` / `src/runtime/symtbl.c` | Per isolate: each loads its own modules (like JS workers). Simple and safe |
| Atom table | `src/common/atom.c` | Shared, with a read-write lock, or per isolate |
| Static types (`kl_fini_static_types`) | Global | Shared read-only, after checking they hold no per-run mutable caches |
| Code objects and JIT code | Per module | Shareable when modules come from the same `.klc`: compile under a lock, keep the chunked code arena (DESIGN §16) per isolate or lock it. `jit_hotness` races are harmless |
| Logger | `src/common/log.c` | Make thread-safe or per isolate |
| `mm_*` block tracking (debug) | Global | Lock in debug builds |

### 10.3 Cross-isolate channels

- Values allowed: numbers, bool, `str`, `bytes`, `nil`, tuples and containers of those (deep
  copied), and frozen objects (shared, never copied, once the frozen family exists).
- Implementation: a mutex-protected queue plus an `eventfd` per receiving isolate, so the receiver's
  poller wakes the parked coroutine. Sending never blocks the sender's thread: if the queue is full,
  the sending coroutine parks on its own isolate's poller.

### 10.4 Why not M:N on one shared heap

It needs all of option A in §3.1: thread-safe allocation, a multi-thread stop-the-world handshake or
fully concurrent marking with atomic `TValue` stores (§9.1), locks or defined races for `list` and
`dict`, and thread-safe native libraries. Coroutines also can't move between threads while inside
native frames, which is the remaining source of "pinning" in Java's virtual threads. Revisit only if
isolates prove too limiting.

---

## 11. JIT impact, phase by phase

| Phase | JIT change |
|---|---|
| 1 | None. JIT code reads `ks->stack_limit` and `ks->c_stack_limit`, which `kl_new_co_ks` sets per coroutine. Verify with the JIT direct-call tests run inside coroutines (`KOALA_JIT_THRESHOLD=1`) |
| 2–3 | None. Parks happen inside helpers (`kl_jit_call_helper` → C function), where `stack_top` is valid (DESIGN §14.1 rule 4) |
| 4 | None. Root scanning is the same per `ks` |
| 5 | Inline `SET_FIELD` gets the barrier check (§9.3). Global stores don't. Helper-based stores get it through their C setters |
| §6.11 (optional) | Preemption check at the OSR back-edge counter (DESIGN §17) |
| Unboxed JIT (Yowie, DESIGN §14.2) | Keep "safepoint = call site". With stack maps or spill-at-call, parked coroutines are scanned the same way as the running one, as long as every park is at a call site with a stack map |
| 6 | Code arena and compile path need a lock if code is shared across isolates |

---

## 12. Debugging and tooling

### 12.1 Tracebacks per coroutine

- `task.dump()` prints every live task with its state and Koala traceback, walking each `ks->cf`
  chain as `trace_here` does today.
- `SIGQUIT` (Ctrl-\\) prints the same, like Go.

### 12.2 Deadlock detection

When `kl_park` finds no runnable coroutine, no registered fds, no timers and no pending offloads,
nothing can ever wake anyone. Print "all tasks are parked" plus `task.dump()` and exit with an
error code (this also makes usage issue #8, exit code 0 on errors, more important to fix).

### 12.3 Statistics

- `KOALA_CO_STATS=1` at exit: tasks spawned, peak live, context switches, average and max park time,
  region pool hits.
- `gc` module (`src/modules/gc.c`): add per-cycle atomic-step pause times, slices, barrier hits
  and finalizer queue length to `GCCycleStats`.

### 12.4 Stress modes

| Variable | Effect |
|---|---|
| `KOALA_GC_STRESS=park` | Full collection on every park (finds invariant-4 violations) |
| `KOALA_GC_STRESS=alloc:N` | Collection every N allocations (after §8.3) |
| `KOALA_GC_VERIFY=1` | Verifier pass at `GC_REMARK` (finds missing barriers, §9.3) |
| `KOALA_CO_SHUFFLE=seed` | Randomizes run-queue order (finds ordering assumptions in tests) |
| `KOALA_CO_TRACE=1` | Logs every spawn, park, wake and switch through `log.c` |

### 12.5 Sanitizers

Fiber annotations (§5.7) for ASan, TSan and Valgrind. Add `test/test-c/tsan_sched.sh` for the
blocking pool and the sweeper thread, modeled on `tsan_wsdeque.sh`.

---

## 13. Risks and mitigations

| Risk | Mitigation |
|---|---|
| C stack too small for deep recursion inside coroutines | Measure per-call C stack use (§5.4); configurable sizes; clean "stack overflow" error through existing checks |
| `vm.max_map_count` caps coroutine count at ~32k | One region per coroutine (§5.3); `KOALA_CO_GUARD=0`; document the sysctl |
| Natives hold unrooted objects across parks (issue #11) | Shadow stack (§8.3); GC only at park points first (§8.4); `KOALA_GC_STRESS=park` |
| Missing write barrier → freed live object | `KOALA_GC_VERIFY`; grep-based audit list (§9.3); barrier helpers `gc_write`/`gc_write_obj` used everywhere |
| CPU-bound task starves others | `task.yield_now()`; optional preemption (§6.11) |
| Blocking native call stalls every task | Offload pool (§7.6); `@parks`-style docs; `KOALA_CO_STATS` reports long gaps between parks |
| O_NONBLOCK leaking to parent shell | Never set it on inherited fds (§7.5) |
| Platform differences (kqueue vs epoll, no `pidfd`) | Poller interface (§7.1); fall back to a `SIGCHLD` self-pipe |
| Sanitizer noise from stack switching | Annotations (§5.7) |

---

## 14. Milestones and tests

All tests go into the existing lit and FileCheck suites (`test/test-run`, `test/test-kl`,
`test/test-c`) unless noted.

### M1: fibers and the scheduler core (Phases 1–2, timers only)

Deliverables: `fiber_x86_64.S`, regions and pool, `kl_new_co_ks`, scheduler, `TaskObject`, `spawn`/`join`,
`Channel`, `group`, `sleep`, cancellation, GC roots from all `ks` (§8.1) with GC only on explicit
`gc.collect()`.

Tests:
- `test_task_basic.kl`: spawn, join, result values of each kind (int, float, ref, intf, optional)
- `test_task_error.kl`: child error re-raised by `join`; unjoined error printed at exit
- `test_channel.kl`: buffered, rendezvous, close, `for v in ch`
- `test_group.kl`: join all; first error cancels siblings; nested groups
- `test_cancel.kl`: cancel while parked, while runnable, before start; `with` cleanup runs
- `test_task_gc.kl`: 10k parked tasks, `gc.collect()`, every task's locals survive and are correct after waking
- `test_task_stack_overflow.kl`: deep recursion in a task gives "stack overflow", other tasks continue
- `test_task_jit.kl`: JIT direct calls and self-only recursion inside tasks, with `KOALA_JIT_THRESHOLD=1`
- `test_deadlock.kl`: all tasks parked on channels, error exit with task dump
- C test `test_fiber.c`: switch and trampoline, MXCSR preserved, alignment
- Memory: RSS for 100k idle tasks (record the number; target under ~2 GB)
- `bench/`: no regression for programs that never spawn

### M2: I/O (Phase 3, sockets, pipes, signals)

Deliverables: poller (epoll first), socket changes (§7.3), pipes and `Process.wait`, signals, stdin
readiness, deadlines.

Tests: echo server with 1k concurrent clients; `libs/koala/http` server accept loop with a group;
`os.run` with large output on several tasks at once (no hang, as in the `communicate()` fixes in
`Koala_TODO.md`); deadline and timeout errors; inherited fds keep blocking mode.

### M3: blocking pool and TLS

Deliverables: `kl_run_blocking`, DNS, sqlite with a per-connection mutex, TLS non-blocking.

Tests: slow sqlite query doesn't block a ticking task; cancel during offload; TLS client against a
local server; `tsan_sched.sh`.

### M4: GC triggers and native roots (Phase 4)

Deliverables: shadow stack revived (§8.3), audit of `os.c`, `socket.c`, `dictobj.c`, `hashset.c`,
`libs/koala/*` natives; automatic GC at park points (4a), then on allocation (4b); escape analysis
rule (§8.5); plain gray stack replacing `gc_gray_dq` (§8.7).

Tests: the full suite under `KOALA_GC_STRESS=park`, then `alloc:1000`; `test_gc_*.kl` with tasks;
`elapsed_ms` of a large-heap collection before and after the gray-stack change.

### M5: incremental GC (Phase 5)

Deliverables: SATB barrier in interpreter, C setters, natives and JIT; lazy stack scanning; pending
free; sweeper thread; finalizer queue; pacing; verifier.

Tests: full suite under `KOALA_GC_VERIFY=1` with small step sizes; differential runs with JIT
on and off; pause-time report from the `gc` module on an allocation-heavy server benchmark; `bench/`
with marking idle shows no measurable change.

### M6: isolates (Phase 6)

Deliverables: `GCHeap` and other globals per isolate (§10.2), cross-isolate channels, `isolate.spawn`.

Tests: parallel CPU benchmark scales with cores; message copy correctness; TSan clean.

---

## 15. Open questions

1. **Generic signature of `spawn`.** `spawn(fn, args...)` vs `spawn(lambda)` only. Depends on how
   well the type checker infers a function value's type with variadic arguments.
2. **Program exit.** Should `main` returning wait for detached tasks (Python asyncio style) or end the
   program (Go style, proposed in §6.10)?
3. **Unjoined task errors.** Print when the task is freed (proposed) or fail the program?
4. **Per-fd concurrency.** One reader and one writer per socket (proposed, as Go) vs a queue of readers.
5. **Default sizes** (§5.4): set after measuring the C stack used per call.
6. **arm64 timing**: write `fiber_arm64.S` in M1 or wait for arm64 JIT support?
7. **Frozen objects for isolates**: depends on the frozen collection family design.

---

## Appendix A: how other languages do it

| Language | Model | Notes for Koala |
|---|---|---|
| Go | Stackful goroutines, M:N, growable (copied) stacks | Copying stacks needs exact maps of every C frame; Koala's C stacks can't be copied, so we use fixed lazy-committed regions instead |
| Java 21+ (Loom) | Virtual threads, stackful, JIT-compiled code | Shows stack switching works with a JIT. Since JDK 24 (JEP 491) the remaining pinning comes from native frames, which isolates avoid |
| PHP 8.1+ | Fibers: full-stack switching in a C VM with a JIT | Closest to this plan: C interpreter, JIT, suspend from anywhere |
| Ruby 3 | Fibers plus a pluggable fiber scheduler that makes blocking I/O non-blocking | Same "existing sync APIs become concurrent" approach as §7 |
| Lua | Coroutines without their own C stack; can't yield across C calls (needs `lua_yieldk` continuations) | Shows why Koala, which recurses on the C stack, needs real C stacks per coroutine |
| Python | asyncio (colored `async`/`await`); free-threaded build supported from 3.14, ~5–10% single-thread cost | Shows the cost of option A and the split between sync and async libraries in option B |
| Rust, C#, JS, Kotlin, Swift | Stackless, compiler state machines, colored functions | Lowest memory per task; needs compiler work and async versions of APIs |
| Zig 0.16 | `std.Io` interface passed by the caller; same code runs blocking, threaded or evented | Same goal as §7: one I/O API for sync and concurrent use |
| OCaml 5 | Effect handlers (fibers) with direct-style I/O (Eio) | Direct style without colors; Ocsigen kept the colored Lwt partly to keep suspension visible, which §6.12 addresses |
| Erlang, Dart, JS workers | Isolated heaps with message passing | Model for §10 |

## Appendix B: glossary

| Term | Meaning |
|---|---|
| Fiber / stackful coroutine | A coroutine with its own stack; it can suspend from any call depth |
| Park | Suspend the current coroutine until something makes it runnable |
| Safepoint | A point where the GC may run and all live references are where the GC looks for them; in Koala, call sites |
| SATB | Snapshot-at-the-beginning marking: everything reachable when the cycle starts survives; overwritten references are shaded by a write barrier |
| Isolate | A VM instance on its own thread with its own heap; isolates share no mutable objects |
| Structured concurrency | Tasks are tied to a lexical scope (`with task.group()`); leaving the scope waits for or cancels them |
