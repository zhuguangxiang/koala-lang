# Koala Playground - 在线运行 Koala 代码

## 快速开始

### 一键启动（推荐）

```bash
./tools/kl-playground.sh
```

这会自动：
1. 启动 playground 服务器
2. 打开浏览器访问 http://localhost:8080
3. 显示文档，代码块有 ▶ Run 按钮

### 手动启动

```bash
# 启动服务器（自动打开浏览器）
python3 tools/kl-playground-server.py

# 不自动打开浏览器
python3 tools/kl-playground-server.py --no-browser

# 自定义端口
python3 tools/kl-playground-server.py --port 9000
```

## 使用方式

1. 服务器启动后，浏览器自动打开 http://localhost:8080
2. 浏览文档，找到 Koala 代码块
3. 点击右上角的绿色 **▶ Run** 按钮
4. 结果显示在代码块下方（绿色=成功，红色=错误）
5. 点击 × 关闭结果面板

## 架构

```
┌─────────────────────────────────────┐
│  kl-playground-server.py            │
│  (单进程，单端口)                     │
│                                     │
│  GET  /         → 静态 HTML 文档     │
│  GET  /manual.html → 语言手册        │
│  POST /run      → 执行 Koala 代码    │
└─────────────────────────────────────┘
```

服务器同时提供：
- 静态文件服务（docs/html/）
- 代码执行 API（POST /run）

## 示例代码

### Hello World
```koala
func main() {
    print("Hello, World!")
}
```

### 变量和计算
```koala
func main() {
    let x = 10
    let y = 20
    print(x + y)
}
```

### 循环
```koala
func main() {
    var sum = 0
    for i in range(1, 11) {
        sum += i
    }
    print("Sum: " + str(sum))
}
```

## 安全特性

- ✅ 仅监听 `localhost`，不对外暴露
- ✅ 执行超时限制 5 秒
- ✅ 临时文件执行后自动删除
- ✅ 防止目录遍历攻击

## 故障排除

### 端口被占用

```bash
# 查看占用端口的进程
lsof -i :8080

# 使用其他端口
python3 tools/kl-playground-server.py --port 9000
```

### 编译错误

确保已构建 koala：
```bash
# 检查二进制文件
ls build/release/bin/koala

# 如需重新构建
./build.sh
```

## API 文档

### POST /run

执行 Koala 代码

**请求**:
```json
{
  "code": "func main() { print(\"hello\") }"
}
```

**响应**:
```json
{
  "output": "hello\n"
}
```

## 统计信息

- **19 个 HTML 文件**
- **1,410 个文档符号**
- **57 个可执行代码块**
- **17 个包的 API 参考**
- **29 个语言手册章节**
