
# Koala Language Manual

Koala是一款小巧，安全，高效且现代的编程语言，面向App开发，高性能计算和AI算子开发。
Koala对开发者非常友好，容易上手，消除了其他编程语言的各种不友好的特性，让编程成为开发者手中的瑞士军刀。
它有如下特点：

1. 所有类型都是静态的，且支持类型推导，用户写起来像`Python`一样流畅，同时是类型安全的。
2. 空（NULL）安全，支持if对可以为空的类型自动检查和解包，对开发者非常友好，也支持if let和while let的显示解包.
3. 对象系统，支持最小集的OOP概念，避免了开发者无序使用导致代码越来越来难维护和扩展。
4. class永远是final的，不支持继承；trait作为接口是可以继承的，这里的继承本质上是组合。
5. Koala的接口是静态派发的，不会查找，所有的接口表都是编译器确定的；同时支持部分接口实现，由VM检测并产生不可恢复的错误。
6. Koala也支持最小集泛型编程。
7. 所有变量使用前必须赋值，没有未定义行为。
8. 错误是强制消除的，没有异常等概念。
9. 内存是半自动管理的，包括手动free和自动释放。
10. 支持C语言扩展，和CPython类似，同时支持有约束的内存自动管理
11. Koala的字节码和VM是基于三地址码和寄存器的
12. 优化器对Koala IR进行了大量的激进的优化，生成高效的VM的字节码
13. 整形溢出是安全的

## Hello World

Koala是可以有main函数的，经典的hello world代码如下：

```swift
func main() {
    print('hello, world')
}
```

Using the below command to compile, it will generate the suffix `klc` byte code file.
> koala -c hello.kl

- Executing the koala without any arguments, and input is `kl` source file.

> koala hello.kl

- Executing the koala without any arguments and input is `klc` byte code file.

> koala hello.klc

## Basics

Koala支持很多基础数据类型，包括整型，浮点型，布尔型和字符串等；同时也支持列表，数组，集合和字典等容器类型。

### 常量和变量

### 注释和文档注释

### 整型

### 浮点型

### 整型转换

### 整型和浮点型转换

### 布尔

### 元组和变长类型

### 可空类型

## 字符串和字符

## 容器类

## 分支和循环

## 函数和闭包

## 类和接口

## 包管理
