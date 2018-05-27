# Develop Release Notes

## Example

```go
package hello;
import "koala/io";

func Main(args []string) {
  io.Println("Hello, World");
}

```

## 类型种类

- [x] 基本类型(int, float, bool)
- [ ] 字符串类型
- [ ] 数组类型
- [ ] 闭包类型(函数也作为一种类型)
- [ ] 类型别名
- [ ] Class&Trait

## 编译

Koala的编译单元为一个.kl源文件，每一个编译单元分为2步：

1. 解析当前源码中的所有包级(public/private)的符号，将符号插入到包级符号表中，并生成抽象语法树(Abstract Syntax Tree)
2. 解析每条语句
3. 生成.klc(**k**oa**l**a **c**ode)二进制文件

## 运行

## Koala工具

1. koala build abc.kl or package-name(directory)

  如果构建的参数是目录，则会编译目录下所有.kl文件，这里要求所有目录下的所有的源码的package和目录名一样，并且会生成一个以目录名为名字的.klc文件

  举例：koala/net目录

    - ping.kl
    - socket.kl
    - http.kl
    - ftp.kl

  net目录下的所有.kl文件都必须`package net;`进行打包

    import "koala/net";

    func test() {
      net.Ping("10.158.233.89");
    }
