# Koala Playground

Koala Playground 是一个本地 HTTP 服务器，用于在浏览器中运行 Koala 代码。

## 文件说明

- `kl-playground-server.py` - Python HTTP 服务器，提供文档服务和代码执行功能
- `kl-playground.sh` - 一键启动脚本
- `koala-playground.js` - 前端 JavaScript，处理代码运行和编辑功能
- `html/` - 生成的 HTML 文档目录

## 使用方法

### 方式一：使用启动脚本（推荐）

```bash
./tools/playground/kl-playground.sh
```

### 方式二：直接运行 Python 服务器

```bash
python3 tools/playground/kl-playground-server.py [--port 8080] [--no-browser]
```

### 参数说明

- `--port PORT` - 指定端口号（默认：8080）
- `--no-browser` - 不自动打开浏览器

## 功能

1. 从 `tools/playground/html/` 提供静态 HTML 文档服务
2. 通过 POST `/run` 端点执行 Koala 代码
3. 启动时自动打开浏览器

## 访问

启动后访问 http://localhost:8080 查看带可运行代码示例的文档。

## 重新生成文档

```bash
python3 tools/kl-html-doc.py
```

## 前置要求

- Python 3.x
- Koala 编译器（`build/release/bin/koala`）