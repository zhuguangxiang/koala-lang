# Koala Desktop - 快速开始

## 🚀 一键安装

```bash
./tools/kl-desktop/install.sh
```

## 🎯 启动应用

```bash
./tools/kl-desktop/kl-desktop.sh
```

## 📦 手动安装

如果一键安装失败，可以手动安装：

### 1. 安装系统依赖（仅 Linux）

```bash
# Ubuntu/Debian
sudo apt install python3-gi python3-gi-cairo gir1.2-gtk-3.0 gir1.2-webkit2-4.0

# Fedora
sudo dnf install python3-gobject gtk3 webkit2gtk3

# Arch
sudo pacman -S python-gobject gtk3 webkit2gtk
```

### 2. 安装 Python 依赖

```bash
pip3 install pywebview
```

### 3. 生成文档（如果还没有）

```bash
python3 tools/kl-html-doc.py --output docs/html
```

### 4. 启动桌面应用

```bash
python3 tools/kl-desktop/kl-desktop-app.py
```

## ✨ 功能特性

- 🖥️ **原生窗口** - 独立的桌面应用，不需要浏览器
- 📚 **完整文档** - 包含语言手册和 API 参考
- 🎮 **集成 Playground** - 直接在应用中运行代码
- 🔍 **搜索功能** - 快速查找文档
- 📌 **侧边栏折叠** - 点击箭头按钮隐藏/显示导航
- 💾 **状态保持** - 记住滚动位置和折叠状态
- 🎨 **现代 UI** - 深色主题，流畅动画

## 🎮 使用技巧

### 侧边栏
- 点击左侧边缘的箭头按钮折叠/展开侧边栏
- 滚动位置会自动保存

### 代码运行
- 点击代码块的 **▶ Run** 按钮运行示例
- 首页有独立的 Playground 区域
- 使用 **Ctrl+Enter** 快速运行代码

### 搜索
- 在搜索框输入关键词过滤 API
- 实时过滤，即时显示结果

## 🐛 故障排除

### PyWebView 安装失败

**Linux:** 确保安装了 GTK 和 WebKit 的开发包
```bash
sudo apt install python3-gi python3-gi-cairo gir1.2-gtk-3.0 gir1.2-webkit2-4.0
```

**macOS:** 应该可以直接安装
```bash
pip3 install pywebview
```

**Windows:** 需要 EdgeChromium 或 MSHTML
```bash
pip3 install pywebview
```

### 端口被占用

应用会自动尝试下一个可用端口（8081, 8082...）

### 文档未找到

应用会自动生成 HTML 文档，如果需要手动生成：
```bash
python3 tools/kl-html-doc.py --output docs/html
```

## 📝 技术栈

- **PyWebView** - 轻量级原生窗口框架
- **Python HTTP Server** - 内置文档服务器
- **HTML/CSS/JS** - 现代化的文档界面
- **highlight.js** - 代码语法高亮

## 🎯 与浏览器版本的区别

| 特性 | 桌面应用 | 浏览器版本 |
|------|---------|-----------|
| 启动方式 | 双击脚本 | 手动启动服务器 |
| 窗口 | 原生窗口 | 浏览器标签页 |
| 依赖 | 需要 PyWebView | 仅需浏览器 |
| 集成度 | 完全集成 | 需要两个步骤 |
| 资源占用 | 较低 | 浏览器开销 |

## 📚 更多信息

查看完整文档：[README.md](README.md)
