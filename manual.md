# Koala语言手册

## Hello World

```go
package main;
import "koala/io";

func Main(args [string]) {
  io.Println("Hello, World");
}
```

> 编译：koala build main.kl
> 运行：koala run main

## 数据类型

- 基本类型(int, float, bool)
- 字符串类型(string)
- 数组类型([int], [[int]], [Person])
- 闭包类型(func(args) rets)
- 类型别名(typealias)
- 类和特质(class, trait)

### 基础数据类型

### 数组和字典形式化描述

&emsp;&emsp;数组和字典是一种复合类型，其成员对象的类型可以理解为一种子类型。
Koala提供了一种形式化描述（编译器内建）。

>
> Question:  
> &emsp;是否需要提供通用的对象描述（数据类型提供）？两种类型是否需要统一的访问模式呢？  
> Answer:  
> &emsp;Koala不支持泛型，但是像数组和字典等容器类的数据类型又需要‘子类型’，这就出现了自相矛盾。Koala解决的办法是：使用形式化的方式描述，通用的容器类对象和形式化方式描述的对象不是同一种类型的对象，不能互相转换。

### 变长数组和固定长度数组

&emsp;&emsp;Koala支持变长数组，不支持固定长度数组，因为数组是一个内建的对象，可以理解为基本数据类型，而不是一个对象。

### 数组

```go
//定义一个整形数组变量，并且创建数组对象
var arr [int];
var arr [int] = [];

//定义一个整形数组变量，并且初始化为一个不为空的数组对象
var arr3 [int] = [1,2,3];
var arr3 = [1,2,3];

//访问数组
arr[0] = 100;
val = arr[0] + arr[1];

//二维数组
var arr4 [[int]] = [[1,2,3], [4,5,6]];
arr4[1][2] = 100; //-->> [[1,2,3], [4,5,100]]
```

### 字典

```go
//定义一个字典变量，并且创建字典对象
var map [string:string];
var map [string:string] = {};

//定义一个字典变量，并且初始化为一个不为空的字典对象
var map [string:string] = {"Name": "James", "Age": "120"};

//访问字典
val = map["Name"];
map["Age"] = "210";

//数组字典嵌套
var data = [{"Name" : "James", "Age": 120, "Male": true}, 100]; // [Any]

```

### 对象

```go
var p1 Person = Person(arg1, arg2);
var p2 = Person(arg1, arg2);
```

### 范型

不少语言，包括Java，Swift等，都支持泛型，也有语言不支持泛型，包括golang，python等，那么为什么要有范型呢？

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

func Main(args [string]) {
  net.Ping("10.158.233.89");
}

koala@Linux:~/koala$koala run app
```

## 汇编的想法

  1. 函数调用栈
  2. 寄存器功能和分配
  3. 通用汇编宏
  4. NASM汇编器
  5. Example

```go
c = a + b;
```

```asm
mov eax, esp
add eax, 18
mov ebx, esp
add ebx, 26
call tvalue_add
mov [sp + 10], eax
``
