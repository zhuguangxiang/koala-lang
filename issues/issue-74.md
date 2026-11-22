# 字符串字面量无 `\u` / `\U` 转义，未知转义静默保留

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 三个讨论点均未落地；当前静默保留未知转义

> 来源：`docs/Koala_TODO.md` §45（2026-11-01，to discuss）

---

### 详细描述

<details>
<summary><b>🔁 最小复现</b></summary>

写 `koala/re` 测试时发现：`"1٣"` 本意是 `1` 加 ARABIC-INDIC DIGIT THREE，但 Koala 没有 `\u` 转义。**反斜杠加一个未知转义字母会原样留在字符串里**，无错误、无警告：

```kl
func main() {
    let a = "1٣"
    print(a.len(), a)      // 7 1٣
    let b = "\U0001F600"
    print(b.len(), b)      // 10 \U0001F600
    let c = "\x41"
    print(c.len(), c)      // 1 A
    let d = "\q"
    print(d.len(), d)      // 2 \q
}
```

</details>

<details>
<summary><b>💥 实际后果</b></summary>

测试随后用 `\d` 搜 `1٣`，找到**五个**数字而非两个；现在改为直接写该字符本身（`"1٣"`）。

手册**没有列出**字符串转义。

</details>

---

### 修复方案

<details>
<summary><b>🛠 三个讨论点</b></summary>

1. 增加 `\uXXXX` 与 `\UXXXXXXXX`（同 Python / Go / C），使非 ASCII 字符可以写在 ASCII 源码里。
2. 把未知转义（如 `\q`）变成**编译错误**（Go、Rust）或至少**告警**（Python 3.12+ 会告警），而不是静默保留。
3. 在手册（Strings and Characters）里记录转义序列。

</details>