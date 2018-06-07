# Koala Language Manual

## Introduction

数组和字典形式化描述

数组：

固定长度VS变长

```go
var arr []strng = [3]string;
arr[0] = string("hello");
arr[1] = "world";

var arr1 []string = ["hello", "world"];
var arr2 = ["hello", "world"]; //[]string
var arr3 = ["hello", "world", 120, Person("ZhangSan")]; //[]Any
var arr4 = [{"Name" : "James", "Age": 120, "Male": true}, 100]; //[]Any
var arr5 [][]int = [[1,2,3], [4,5], [6]];

```
字典:

```go
var dict [string:string] = [string:string]();
dict["name"] = "James";
dict["age"] = "20";

var dict2 [string:Any] = {"Name" : "James", "Age": 120, "Male": true};
var dict3 = {"Name" : "James", "Age": 120, "Male": true};
```

对象：

```go
var o1 Person = Person(arg1, arg2);
var o2 = Person(arg1, arg2);
```

范型：
为什么要有范型？

## Example

```go
package hello;
import "koala/io";

func Main(args []string) {
  io.Println("Hello, World");
}
```

## 类型种类

Array Type:

```go
var persons []Person = [3]Person{p1, p2, p3};
var persons []Person = {{"Name": "James", "Age" : 121}, {}, {}};
var personMap = {"Name": "James", "Age" : 121};
```

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

  举例：net目录下的所有.kl文件都必须`package net;`进行打包

    -net
      |--ping.kl
      |--socket.kl
      |--http.kl
      |--ftp.kl

  构建：

    koala@Linux:~/koala$koala build net

  输出：

    koala@Linux:~/koala$net.klc
    koala@Linux:~/koala$cp net.klc ~/.koala-repo/koala.org/koala/

  部署:

    koala@Linux:~/koala$koala install net koala.org/koala

  使用：

```go
//app.kl
package app;
import "koala.org/koala/net";

func Main(args []string) {
  net.Ping("10.158.233.89");
}

koala@Linux:~/koala$koala run app
```
