# Koala `koala/random` API Reference

## Contents

**Functions**

| Name | Summary |
|------|---------|
| [`random`](#random) | Return a random float in the range [0.0, 1.0). |
| [`randint`](#randint) | Return a random integer in the range [a, b] (inclusive). |
| [`randrange`](#randrange) | Return a random integer in the range [0, stop). |
| [`randrange_step`](#randrange_step) | Return a random integer in the range [start, stop) with the given step. |
| [`uniform`](#uniform) | Return a random float in the range [a, b]. |
| [`seed`](#seed) | Set the random seed for reproducibility. |
| [`choice`](#choice) | Return a random element from a non-empty sequence. |
| [`shuffle`](#shuffle) | Shuffle a mutable sequence in place. |
| [`sample`](#sample) | Return a list of `k` unique elements sampled from the sequence. |

## Functions

<a id="random"></a>

### `random`

```kl
pub func random() float
```

*`@native`*

Return a random float in the range [0.0, 1.0).

<a id="randint"></a>

### `randint`

```kl
pub func randint(a int, b int) int
```

*`@native`*

Return a random integer in the range [a, b] (inclusive).

Panic if `a > b`.

<a id="randrange"></a>

### `randrange`

```kl
pub func randrange(stop int) int
```

*`@native`*

Return a random integer in the range [0, stop).

Panic if `stop <= 0`.

<a id="randrange_step"></a>

### `randrange_step`

```kl
pub func randrange_step(start int, stop int, step int) int
```

*`@native`*

Return a random integer in the range [start, stop) with the given step.

Panic if `step == 0` or if the range is empty.

<a id="uniform"></a>

### `uniform`

```kl
pub func uniform(a float, b float) float
```

*`@native`*

Return a random float in the range [a, b].

Panic if `a > b`.

<a id="seed"></a>

### `seed`

```kl
pub func seed(s int)
```

*`@native`*

Set the random seed for reproducibility.

<a id="choice"></a>

### `choice`

```kl
pub func choice[T](seq Sequence[T]) T
```

Return a random element from a non-empty sequence.

Panic if the sequence is empty.

<a id="shuffle"></a>

### `shuffle`

```kl
pub func shuffle[T](seq MutableSequence[T])
```

Shuffle a mutable sequence in place.

<a id="sample"></a>

### `sample`

```kl
pub func sample[T](seq Sequence[T], k int) list[T]
```

Return a list of `k` unique elements sampled from the sequence.

Panic if `k` is negative or greater than the sequence length.
