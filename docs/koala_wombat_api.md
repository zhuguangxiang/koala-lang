# Koala `koala/wombat` API Reference

## Contents

**Classes**

| Name | Summary |
|------|---------|
| [`Package`](#package) | A package in the workspace. |
| [`Workspace`](#workspace) | The workspace being built. |

**Functions**

| Name | Summary |
|------|---------|
| [`build_all`](#build_all) | Load a workspace or a single package and build it. |
| [`load_workspace_or_package`](#load_workspace_or_package) | Load a workspace if `__ws__.toml` exists, or a single-package workspace if `__pkg__.toml` exists instead. |
| [`load_single_package`](#load_single_package) | Build a workspace-like structure from a single package directory. |
| [`toml_str`](#toml_str) | Read a string field, returning "" when absent. |
| [`toml_str_list`](#toml_str_list) | Read a string-array field. |
| [`join_or_none`](#join_or_none) | Join `xs` with `sep`, or "None" if empty. |
| [`load_workspace`](#load_workspace) | Load `<root>/__ws__.toml` and each package's `__pkg__.toml`. |
| [`load_package`](#load_package) | Read `<root>/<dir_name>/__pkg__.toml`. |
| [`find_package`](#find_package) |  |
| [`topo_sort`](#topo_sort) | Topological sort by `dependencies`. Empty on cycle. |
| [`print_dep_map`](#print_dep_map) |  |
| [`print_order`](#print_order) |  |
| [`check_natives`](#check_natives) |  |
| [`artifact_rel`](#artifact_rel) | Artifact path: `<ws>/<dir>.klc`. |
| [`src_rel`](#src_rel) | Source path: `<ws>/<dir>`, relative to the workspace's parent. |
| [`build_cmd`](#build_cmd) |  |
| [`build_workspace`](#build_workspace) | Compile every package in dependency order. |
| [`base_name`](#base_name) | Basename of `path`: "demo/math" -> "math". |
| [`rel_path`](#rel_path) | Path of `path` relative to `base` when `path` lives under it. |
| [`write_ws_toml`](#write_ws_toml) | Write `<ws_dir>/__ws__.toml` from a TOML object. |
| [`add_ws_package`](#add_ws_package) | Append `entry` to the `packages` list in `<ws_dir>/__ws__.toml`. No-op when already present. |
| [`write_build_script`](#write_build_script) | Write `build-and-test.sh` into `ws_dir` and make it executable. |
| [`new_workspace`](#new_workspace) | Create `<name>/` with a minimal `__ws__.toml` and `build-and-test.sh`. |
| [`new_package`](#new_package) | Create `<name>/` with `__pkg__.toml` and an entry `.kl` that prints "hello world". |
| [`new_auto`](#new_auto) | Create a workspace when the current directory has no `__ws__.toml`, otherwise create a package inside it. |
| [`find_ws_root`](#find_ws_root) | Walk up from `start` until a directory containing `__ws__.toml` is found. Returns "" when none. |
| [`resolve_ws_package`](#resolve_ws_package) | If `name` matches a package in the workspace at `ws_root`, return `<ws_root>/dist/<ws_name>/<name>.klc`. Otherwise return "". |
| [`run`](#run) | `wombat run <target> [args...]` |
| [`find_dist_dir`](#find_dist_dir) | Walk up from `start` until a directory containing `__ws__.toml` or `__pkg__.toml` is found, then return its `dist/` directory. |
| [`quote_arg`](#quote_arg) | Append `s` to `buf`, wrapped in single quotes if it contains characters the shell would otherwise split or interpret. |
| [`test_all`](#test_all) | Run tests for every package in a workspace or a single package. |

## Classes

<a id="package"></a>

### `Package`

```kl
class Package {
    var name = ""
    var dir_name = ""
    var test = "test"
    var version = "0.1.0"
    var files = list[str]()
    var excludes = list[str]()
    var deps = list[str]()
    var natives = list[str]()
    var compiler = ""
    var options = list[str]()
}
```

A package in the workspace.

---

**Fields**

<a id="package.name"></a>

**`var name = ""`**

<a id="package.dir_name"></a>

**`var dir_name = ""`**

<a id="package.test"></a>

**`var test = "test"`**

<a id="package.version"></a>

**`var version = "0.1.0"`**

<a id="package.files"></a>

**`var files = list[str]()`**

<a id="package.excludes"></a>

**`var excludes = list[str]()`**

<a id="package.deps"></a>

**`var deps = list[str]()`**

<a id="package.natives"></a>

**`var natives = list[str]()`**

<a id="package.compiler"></a>

**`var compiler = ""`**

<a id="package.options"></a>

**`var options = list[str]()`**

<a id="workspace"></a>

### `Workspace`

```kl
class Workspace {
    var name = ""
    var version = "0.1.0"
    var root = "."
    var packages = list[Package]()
}
```

The workspace being built.

---

**Fields**

<a id="workspace.name"></a>

**`var name = ""`**

<a id="workspace.version"></a>

**`var version = "0.1.0"`**

<a id="workspace.root"></a>

**`var root = "."`**

<a id="workspace.packages"></a>

**`var packages = list[Package]()`**

## Functions

<a id="build_all"></a>

### `build_all`

```kl
pub func build_all(src_dir str, output_dir str) int
```

Load a workspace or a single package and build it.

`src_dir` may contain `__ws__.toml` (workspace mode) or
`__pkg__.toml` (single-package mode). A relative `output_dir` is
resolved against the workspace root in workspace mode, and against
the package directory in single-package mode.

<a id="load_workspace_or_package"></a>

### `load_workspace_or_package`

```kl
func load_workspace_or_package(root str) Workspace
```

Load a workspace if `__ws__.toml` exists, or a single-package
workspace if `__pkg__.toml` exists instead.

<a id="load_single_package"></a>

### `load_single_package`

```kl
func load_single_package(pkg_dir str) Workspace
```

Build a workspace-like structure from a single package directory.

`ws.root` is the package's parent, so `src_rel` yields
`<parent>/<pkg>` and `artifact_rel` yields `<parent>/<pkg>.klc`,
the same layout workspace mode would produce.

Dependencies are cleared: a single-package build does not resolve
or order them. Packages the target imports must already be on
`KOALA_PATH`, produced by a prior workspace or single-package
build.

<a id="toml_str"></a>

### `toml_str`

```kl
func toml_str(tbl toml.Table, key str) str
```

Read a string field, returning "" when absent.

<a id="toml_str_list"></a>

### `toml_str_list`

```kl
func toml_str_list(tbl toml.Table, key str) list[str]
```

Read a string-array field.

<a id="join_or_none"></a>

### `join_or_none`

```kl
func join_or_none(xs list[str], sep str) str
```

Join `xs` with `sep`, or "None" if empty.

<a id="load_workspace"></a>

### `load_workspace`

```kl
func load_workspace(root str) Workspace
```

Load `<root>/__ws__.toml` and each package's `__pkg__.toml`.

<a id="load_package"></a>

### `load_package`

```kl
func load_package(root str, dir_name str) Package
```

Read `<root>/<dir_name>/__pkg__.toml`.

<a id="find_package"></a>

### `find_package`

```kl
func find_package(ws Workspace, name str) Package
```

<a id="topo_sort"></a>

### `topo_sort`

```kl
func topo_sort(pkgs list[Package]) list[str]
```

Topological sort by `dependencies`. Empty on cycle.

<a id="print_dep_map"></a>

### `print_dep_map`

```kl
func print_dep_map(ws Workspace)
```

<a id="print_order"></a>

### `print_order`

```kl
func print_order(ws Workspace, order list[str])
```

<a id="check_natives"></a>

### `check_natives`

```kl
func check_natives(pkg Package, ws Workspace) int
```

<a id="artifact_rel"></a>

### `artifact_rel`

```kl
func artifact_rel(ws Workspace, pkg Package) str
```

Artifact path: `<ws>/<dir>.klc`.

<a id="src_rel"></a>

### `src_rel`

```kl
func src_rel(ws Workspace, pkg Package) str
```

Source path: `<ws>/<dir>`, relative to the workspace's parent.

<a id="build_cmd"></a>

### `build_cmd`

```kl
func build_cmd(pkg Package, ws Workspace, output_dir str) str
```

<a id="build_workspace"></a>

### `build_workspace`

```kl
func build_workspace(ws Workspace, output_dir str) int
```

Compile every package in dependency order.

<a id="base_name"></a>

### `base_name`

```kl
func base_name(path str) str
```

Basename of `path`: "demo/math" -> "math".

<a id="rel_path"></a>

### `rel_path`

```kl
func rel_path(base str, path str) str
```

Path of `path` relative to `base` when `path` lives under it.

<a id="write_ws_toml"></a>

### `write_ws_toml`

```kl
func write_ws_toml(ws_dir str, name str, version str, entries list[str]) int
```

Write `<ws_dir>/__ws__.toml` from a TOML object.

The file is fully regenerated; comments and custom fields are not
preserved. `__ws__.toml` is a tool-maintained file.

<a id="add_ws_package"></a>

### `add_ws_package`

```kl
func add_ws_package(ws_dir str, entry str) int
```

Append `entry` to the `packages` list in `<ws_dir>/__ws__.toml`.
No-op when already present.

<a id="write_build_script"></a>

### `write_build_script`

```kl
func write_build_script(ws_dir str) int
```

Write `build-and-test.sh` into `ws_dir` and make it executable.

Every line goes through `Buffer.write_str`, so `$`, `{`, `(` inside
the shell script are never interpreted by Koala.

<a id="new_workspace"></a>

### `new_workspace`

```kl
pub func new_workspace(name str) int
```

Create `<name>/` with a minimal `__ws__.toml` and
`build-and-test.sh`.

<a id="new_package"></a>

### `new_package`

```kl
pub func new_package(name str) int
```

Create `<name>/` with `__pkg__.toml` and an entry `.kl` that
prints "hello world".

The current directory is treated as the workspace root; the
package's path relative to it is appended to `__ws__.toml`.

<a id="new_auto"></a>

### `new_auto`

```kl
pub func new_auto(name str) int
```

Create a workspace when the current directory has no
`__ws__.toml`, otherwise create a package inside it.

<a id="find_ws_root"></a>

### `find_ws_root`

```kl
func find_ws_root(start str) str
```

Walk up from `start` until a directory containing `__ws__.toml` is
found. Returns "" when none.

<a id="resolve_ws_package"></a>

### `resolve_ws_package`

```kl
func resolve_ws_package(ws_root str, name str) str
```

If `name` matches a package in the workspace at `ws_root`, return
`<ws_root>/dist/<ws_name>/<name>.klc`. Otherwise return "".

<a id="run"></a>

### `run`

```kl
pub func run(target str, args list[str]) int
```

`wombat run <target> [args...]`

`<target>` resolution order:

1. `<target>.kl` / `<target>.klc` as given.
2. If `<target>` is a package name in the current workspace, the
   built artifact `<ws>/dist/<ws>/<target>.klc`.
3. `<target>.klc`, then `<target>.kl` in the current directory.

The workspace's `dist/` directory is prepended to `KOALA_PATH` for
the child process, so the program can `import` packages built by
`wombat build` without the caller setting `KOALA_PATH` by hand.

<a id="find_dist_dir"></a>

### `find_dist_dir`

```kl
func find_dist_dir(start str) str
```

Walk up from `start` until a directory containing `__ws__.toml` or
`__pkg__.toml` is found, then return its `dist/` directory.

<a id="quote_arg"></a>

### `quote_arg`

```kl
func quote_arg(buf Buffer, s str)
```

Append `s` to `buf`, wrapped in single quotes if it contains
characters the shell would otherwise split or interpret.

<a id="test_all"></a>

### `test_all`

```kl
pub func test_all(src_dir str) int
```

Run tests for every package in a workspace or a single package.

Does not build. The output directory is prepended to `KOALA_PATH`
for each test process, so tests can import the `.klc` artifacts
produced by a prior `wombat build`. The parent shell's environment
is not touched.

A package has tests when `<dir>/<pkg.test>` exists. An empty test
directory is reported and skipped.
