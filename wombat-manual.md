# wombat User Manual

`wombat` is the build system and package manager for Koala application developers. It manages workspaces, packages, builds, tests, and runs programs.

---

## 1. Installation

Build and install from the Koala source tree with CMake:

```bash
cmake -S . -B build
cmake --build build
cmake --install build
```

The default install layout places the toolchain under `~/.local/lib/koala-<version>/` and the shell completion script under `~/.local/share/bash-completion/completions/wombat`.

Add the toolchain to PATH:

```bash
export PATH="$HOME/.local/lib/koala-<version>/bin:$PATH"
```

Verify:

```bash
wombat version
```

## 2. Quick Start

```bash
cd /tmp
wombat new demo          # create a workspace in ./demo
cd demo

wombat new math          # create package demo/math
wombat new app           # create package demo/app

wombat build             # build every package in the workspace
wombat test              # run tests for every package
wombat run app           # run the demo/app package
```

After `wombat new math` and `wombat new app`, the workspace file is updated automatically:

```toml
[workspace]
name = "demo"
version = "0.1.0"
packages = ["math", "app"]
```

Users do not need to edit `__ws__.toml` to register new packages.

## 3. Workspace and Package Model

A **workspace** is a directory containing `__ws__.toml`.

A **package** is a directory containing `__pkg__.toml`.

Source files for a package are the `*.kl` files directly inside that directory. `koala -c <dir>` does not recurse into subdirectories, so a subdirectory is not part of the parent package.

### 3.1 Directory layout

```
demo/
├── __ws__.toml
├── math/
│   ├── __pkg__.toml
│   ├── math.kl
│   └── test/
│       └── test_math.kl
├── app/
│   ├── __pkg__.toml
│   ├── app.kl
│   └── test/
│       └── test_app.kl
└── dist/
    └── demo/
        ├── math.klc
        └── app.klc
```

### 3.2 \_\_ws__.toml

```toml
[workspace]
name = "demo"
version = "0.1.0"
packages = ["math", "app"]
```

| Field | Meaning |
|-------|---------|
| name | Workspace name. Also the first component of package names created by `wombat new`. |
| version | Workspace version. |
| packages | Directory paths relative to the workspace root. Each entry must contain `__pkg__.toml`. Maintained by `wombat new`. |

### 3.3 \_\_pkg__.toml

```toml
[package]
name = "demo/math"
version = "0.1.0"
dependencies = ["demo/builtin"]
natives = []
```

| Field | Meaning |
|-------|---------|
| name | Logical package name. Also the import path, and determines the artifact path. |
| version | Package version. |
| dependencies | Package names this package depends on. Used for topological build order. |
| natives | Native libraries required at runtime. |
| test | Optional. Name of the test subdirectory. Defaults to `test`. |

The package name and directory path are normally identical, but the artifact path is determined by `name`, not by the directory name.

### 3.4 Package names and artifact paths

A package named `demo/math` is imported as:

```koala
import "demo/math"
```

Its artifact is:

```
<workspace-root>/dist/demo/math.klc
```

`wombat build` prepends `<workspace-root>/dist/` to `KOALA_PATH` for child processes, so the import resolves without manual configuration.

## 4. Commands

### 4.1 wombat new \<name\>

Creates a workspace or a package.

| Current directory contains | Result |
|---------------------------|--------|
| `__ws__.toml` | Creates package `<name>/` with `__pkg__.toml` and `__<name>__.kl`, and appends the package to `packages`. |
| No `__ws__.toml` | Creates workspace `<name>/` with `__ws__.toml` and `build-and-test.sh`. |

Examples:

```bash
wombat new demo
cd demo
wombat new math
```

A newly created package contains:

```
math/
├── __pkg__.toml
└── __math__.kl
```

`__math__.kl` contains:

```koala
print("hello world")
```

`wombat new` does not create a `test/` directory. Add one when tests are needed.

### 4.2 wombat build [dir] [-o|--output \<dir\>]

Builds a workspace or a single package.

| dir contains | Mode | Behavior |
|--------------|------|----------|
| `__ws__.toml` | workspace | Builds every package in `packages`, in dependency order. |
| `__pkg__.toml` | single package | Builds only that package. |
| neither | — | Prints an error and exits non-zero. |

`dir` defaults to `.`. `-o` / `--output` defaults to `./dist`. A relative output directory is resolved against the workspace root.

Examples:

```bash
cd demo
wombat build
wombat build -o ./build
wombat build math
wombat build examples/demo
```

If any package fails to compile, `wombat build` stops immediately and exits non-zero. Later packages are not built.

### 4.3 wombat test [target]

Runs tests. Does not build; run `wombat build` first.

| target | Behavior |
|--------|----------|
| directory with `__ws__.toml` | Tests every package in `packages`. |
| directory with `__pkg__.toml` | Tests only that package. |
| `.kl` or `.klc` file | Runs `koala --test <file>` directly. |
| omitted | Uses the current directory. |

A package has tests when its `test` directory exists. The directory name defaults to `test` and can be overridden by `test` in `__pkg__.toml`. Every `.kl` file in that directory is run as a separate test process.

Examples:

```bash
cd demo
wombat test
wombat test math
wombat test app/test/test_app.kl
```

`wombat test` injects `<workspace-root>/dist/` into `KOALA_PATH` for each test process, so tests can import packages built by `wombat build`.

### 4.4 wombat run \<target\> [args...]

Runs a `.kl` source file, a `.klc` artifact, or a package.

`<target>` is resolved in this order:

1. If it ends with `.kl` or `.klc`, use it as-is.
2. If it is a package name in the current workspace, use `<workspace-root>/dist/<workspace-name>/<target>.klc`.
3. If `<target>.klc` exists, use it.
4. If `<target>.kl` exists, use it.
5. Otherwise print an error and exit non-zero.

Arguments after `<target>` are forwarded to the program unchanged.

Examples:

```bash
cd demo
wombat run app
wombat run math
wombat run app --port 8080
wombat run app.kl
wombat run dist/demo/app.klc
```

`wombat run` does not build. The package artifact must already exist.

Output is produced by the program itself. A library package without top-level statements exits silently with status 0.

### 4.5 wombat clean

Not implemented yet.

### 4.6 wombat help [command]

```bash
wombat help
wombat help build
wombat help test
wombat help run
wombat help new
```

### 4.7 wombat version

```bash
wombat version
```

## 5. Environment

### 5.1 KOALA_PATH

`KOALA_PATH` is the only lookup path used by `koala` to resolve imports.

`wombat build`, `wombat test`, and `wombat run` prepend the workspace output directory to `KOALA_PATH` for child processes:

```
<workspace-root>/dist/:<existing KOALA_PATH>
```

The parent shell's `KOALA_PATH` is never modified.

### 5.2 LD_LIBRARY_PATH

If a package declares `natives`, `wombat build` searches standard library directories and `<package>/native/` for `lib<name>.so`. Runtime library lookup is handled by the system loader; set `LD_LIBRARY_PATH` if the library is outside standard paths.

## 6. Shell Completion

The completion script is installed as:

```
~/.local/share/bash-completion/completions/wombat
```

If `bash-completion` is installed, it is loaded automatically on first use. Otherwise, source it manually:

```bash
source ~/.local/share/bash-completion/completions/wombat
```

If another completion framework (for example `fzf`) overrides `wombat`, re-source the wombat completion at the end of `~/.bashrc`:

```bash
if [ -f "$HOME/.local/share/bash-completion/completions/wombat" ]; then
    . "$HOME/.local/share/bash-completion/completions/wombat"
fi
```

Completion behavior:

| Input | Candidates |
|-------|-----------|
| `wombat <TAB>` | `new build check run test fmt doc clean help version` |
| `wombat build <TAB>` | directories containing `__ws__.toml` or `__pkg__.toml` |
| `wombat build examples/<TAB>` | workspace or package directories inside `examples/` |
| `wombat test <TAB>` | package directories and `*.kl` / `*.klc` files |
| `wombat run <TAB>` | package directories and `*.kl` / `*.klc` files |
| `wombat run app <TAB>` | no candidates (program arguments) |
| `wombat new <TAB>` | no candidates |

When no candidate matches, `build`, `test`, and `run` fall back to the default filename completion. `new` never falls back.

## 7. Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Build or test failure; runtime failure |
| 2 | Usage error: missing argument, unknown option, unknown command |
| other | `wombat run` forwards the program's exit code |

## 8. Examples

### 8.1 Workspace with a dependency

```bash
cd /tmp
wombat new demo
cd demo

wombat new math
wombat new app

# edit app/__pkg__.toml:
# dependencies = ["demo/math"]

wombat build
wombat test
wombat run app
```

### 8.2 Single package

```bash
cd demo/math
wombat build
```

### 8.3 Running a source file

```bash
cd demo
wombat run app/test/test_app.kl
```

### 8.4 Passing arguments to a program

```bash
cd demo
wombat run app --port 8080 --verbose
```

## 9. Current Limitations

| Area | Status |
|------|--------|
| `wombat clean` | Not implemented. |
| `wombat fmt`, `wombat doc`, `wombat check` | Not implemented. |
| Nested packages | Not yet fully supported. |
| Automatic dependency scanning | Not implemented. `dependencies` must be written manually. |
| Parallel builds | Not implemented. |
| External dependency versions | Not supported. |

## 10. Summary

- `wombat new <name>` creates a workspace or package and maintains `packages` automatically.
- `wombat build [dir]` builds a workspace or a single package.
- `wombat test [target]` runs tests without building.
- `wombat run <target> [args...]` runs a file, artifact, or package.
- `KOALA_PATH` is the single import lookup path; `wombat` injects the workspace `dist/` directory for child processes.
