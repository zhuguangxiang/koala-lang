
# Koala API Design Specification

(Return‑Type Semantics: T → T? → panic)

## Design Goals

Koala’s API design emphasizes:

- Predictability — functions behave consistently and without surprises
- Type‑driven semantics — return types communicate success or failure
- Minimal panic — panics are reserved for unrecoverable errors
- Readable signatures — callers can understand behavior from the type alone

## Tier 1 — Prefer Returning T (Total Functions)

A function should return a plain value T when:

- the operation always succeeds,
- no failure mode is expected in normal usage,
- the function is mathematically total.

Examples

- `len() → int`
- `range.__str__() → str`
- `map.size() → int`

Rule

> If a function can be defined as always successful, it must return T.
>

## Tier 2 — Use T? for Normal, Expected Failure

A function should return an optional T? when:

- failure is a normal and expected outcome,
- the caller can reasonably handle the failure,
- the failure does not indicate a programming error.

Examples

- `map.get(key) → V?`
- `list.get(index) → T?`
- `int.parse(str) → int?`
- `range.index(value) → int?`

Rule

> If failure is part of normal control flow, return T? instead of panicking.



## Tier 3 — Panic Only for Unrecoverable Errors

A function may panic only when:

- an internal invariant is violated,
- the error indicates a programmer mistake,
- continuing execution would be unsafe or meaningless.

**Examples**

- out‑of‑bounds access in `list.at(i)`
- division by zero
- internal runtime invariants
- unreachable states

**Rule**

> Panic is reserved for unrecoverable errors and programmer mistakes.

## 5. Paired API Pattern: Safe + Panic Variants

For operations that may fail, Koala encourages providing **paired APIs**:

- **Safe version** returning `T?`
- **Convenience version** that panics on failure

**Examples**

- `map.get(key) → V?`
- `map.get_or_panic(key) → V`
- `list.get(i) → T?`
- `list.at(i) → T`

**Rule**

> Panic variants must be explicitly named to signal risk.

## Naming Conventions

- Safe optional-returning functions use **plain names**: `get`, `find`, `parse`, `index`
- Panic variants must include an explicit marker: `get_or_panic`, `at`, `unwrap`, `expect`

## Summary

Koala’s API design follows a strict hierarchy:

1. **T** — total, always succeeds
2. **T?** — normal, expected failure
3. **panic** — unrecoverable, programmer error

This ensures:

- predictable behavior,
- explicit failure handling,
- minimal runtime surprises,
- clean and expressive APIs.
