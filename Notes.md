
# 随手笔记
## 语言虚拟机
### 指令集
### 执行状态机
### 对象模型
对象分为managed对象和非managed对象, 前者由垃圾回收算法分配和释放, 用户无需关心其生命周期; 后者为用户自己管理

## 内存管理
### 垃圾回收算法

## Linux-64地址空间

支持46位(64TB)的物理地址空间和47位(128T)的进程虚拟地址空间, 高18位没有使用
[Using the extra 16 bits in 64-bit pointers](https://stackoverflow.com/questions/16198700/using-the-extra-16-bits-in-64-bit-pointers)

## JVM中的G1算法
使用46位中的高4位, 低42位支持4TB地址空间.


## JVM中的ZGC算法

[理解并应用JVM垃圾收集器-ZGC](https://zhuanlan.zhihu.com/p/105921339 "理解并应用JVM垃圾收集器")

## 疑问

### 为什么需要2个survivor区域?

## 资料
- [Koka](https://koka-lang.github.io/koka/doc/index.html, "A Functional Language with Effect Types and Handlers")
- [mimalloc](https://github.com/microsoft/mimalloc)
- handle SIGSEGV nostop noprint