# 带显式类型实参的重复调用缺少 `make_intf`

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 正确的代码在第二次调用时运行期崩溃；根因未定位

> 来源：`docs/Koala_TODO.md` §71（2026-11-07，to fix）

---

### 详细描述

<details>
<summary><b>🔁 最小复现</b></summary>

```kl
trait Maker { func make() Sized }

class Thing : Sized { pub func len() int { return 7 } }

class Narrower : Maker { pub func make() Sized { return Thing() } }

func via_bound[T: Maker](m T) int { return m.make().len() }

func main() {
    print(via_bound[Narrower](Narrower()))
    print(via_bound[Narrower](Narrower()))
}
```

```text
vm_ops.h:430: _eval_frame: Assertion '((callable)->tag > (64 + 1))' failed.
```

</details>

<details>
<summary><b>🔍 现状</b></summary>

第二次调用的 IR 是 `call @via_bound, %4 Narrower`，**前面没有 `make_intf`**。

规律：**只要该函数之前已有任意一次调用**（推断的或显式的），后续的显式调用就会失败。只有第一次显式调用与所有推断调用能拿到 `make_intf`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待定位</b></summary>

**根因尚未追查**（原记录原文 "The cause has not been traced."）。

方向：检查泛型函数实例化 / 调用点缓存 —— 首次调用时把 `make_intf` 的发射记在某处，后续复用旧 IR 时漏掉了这次实参转换。

</details>