# Koala `std/sys` API Reference

## Contents

**Functions**

| Name | Summary |
|------|---------|
| [`args`](#args) | Get the list of command-line arguments passed to the program. Returns an empty list if no arguments are provided. |
| [`exit`](#exit) | Terminate the process with exit code `code`. |

## Functions

<a id="args"></a>

### `args`

```kl
pub func args() list[str]
```

*`@native`*

Get the list of command-line arguments passed to the program.
Returns an empty list if no arguments are provided.

<a id="exit"></a>

### `exit`

```kl
pub func exit(code = 0)
```

*`@native`*

Terminate the process with exit code `code`.
