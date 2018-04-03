## 编程约束：
对外开放的接口：大写_下划线 或者 小写_下划线
内部接口：static 小写 或者 static __xx
判断为NULL或非NULL: if (!ptr) or if (ptr)
使用TAB作为为缩进，默认TAB为2空格长度
入参检查：尽量减少判空，可以是调用者负责，也可以是函数提供者负责
CALLBACK: __xxx_fn
Init&Fini: Init_xxx, Fini_xxx, xxx_Init, xxx_Fini, xxx_Initialize, xxx_Finalize

## libkoala.so：
1. 类型子系统
    1. 模块管理
    2. 符号管理
    3. 语义分析(类型检查)
2. 运行子系统
    1. 协程
    2. 线程
    3. GC
3. 编译子系统
    1. AST
    2. 解析器
    3. 模块管理
    4. 符号管理
    5. 语义分析(类型检查)
4. Koala Code(.klc)

## libs
1. libts.so
2. libmod.so
3. librt.so
4. libcommon.so
5. libcore.so

i,l -> int
f,d -> float
z -> bool
s -> string
v -> void
O -> Object
[ -> array
-----
A -> any
-----
i[A[Okoala/lang.String;vzs

## 类
var s = "hello";
var klazz = String.class or s.GetClass() or Class.ForName("koala/lang.String")

## 包管理

## 函数调用
|  object  |
|  arg0-N  |

## 编译
编译文件
编译包
编译项目
安装
运行
部署

## 目录
bin/
 |--tools/
 |   |--compile
 |   |--disassemble
 |   |--pack
 |   |--install
 |   |--get
 |   |--put
 |   `--run
 |--koala
doc/
cfg/
 |--settings.cfg
src/
Makefile

-----------------------------------------------------------------------------
settings.cfg
repository=~/koala_repo

```

project
    |--verson.cfg
    |--src
    |   |--koala/
    |   |   |--lang/
    |   |       |--string.kl
    |   |       |--klass.kl
    |   |       |--person.kl
    |   |       `--greeting.kl
    |   `--demo.kl
    |--target
    |   |--koala/
    |   |   |--lang/
    |   |       |--string.klc
    |   |       |--klass.klc
    |   |       |--person.klc
    |   |       `--greeting.klc
    |   `--demo.klc
    |--pkg
    |   |--koala
    |   |   |--lang.klc
    |   `--demo.klc

单文件编译(找不到符号报错，指定源文件和目标文件)
compile src/koala/lang/person.kl -o target/koala/lang/person.klc -kpath target/
compile src/koala/lang/greeting.kl -o target/koala/lang/greeting.klc -kpath target/
package target/koala/lang/* -o pkg/koala/lang.klc

单文件编译
compile src/demo.kl -o target/demo.klc
package target/demo.klc -o target/demo.klc

run demo(.klc) 123 "abc" -kpath pkg/

编译查找顺序：
 內建
 当前project下的target
 当前project下的源码
 repo

koala build
同一个目录下的原文件的包一样，Java也是，golang也是
 1. 强制package名和目录名一样
 2. 强制项目结构
 3. 同一个包的不同文件一起编译
 4. 不同包的相互依赖，同时编译多个包(共用编译环境)
    import，函数原型，变量类型，结构体类型
    编译进行多次编译，第一遍无法解析的语句，放到第二遍解析
    编译环境包含多个包，可以互相访问符号
    不同包之间是相互交叉编译

koala install
 1. 复制文件到本地repo中

koala run demo(.klc) 123 "abc" -kpath pkg/

verson.cfg
 koala/lang@0.1.2
 hello@1.2

虚拟机加载按照.klc文件为单位进行加载
编译一个kl文件的时候，如果依赖同一个包下的其他kl文件，则同时编译这2个文件???
  编译单个文件，不自动编译其依赖文件??
    同一个包，自动编译?? 如何判断是同一个包呢?编译哪个文件呢?
      找不到本地符号报错(同包的其他文件中的符号也认为是本地符号)
    不同包呢?? 自动编译外部包，根据import "koala/lang"
    不允许循环依赖(包内和不同包都不允许出现循环依赖)
  编译包，同时编译包下的所有kl文件，共享编译环境
  编译包，外部包依赖??? 不编译，找不到就报错，不允许相互依赖
```

同一个包可以放在不同路径上，使用方法和其他包一样,导入再使用
版本使用git的tag进行管理
install自动打tag，版本已经存在自动覆盖
编译器和虚拟机版本号
  编译器可以有多个compile同时发布，koala选择不同的版本
  虚拟机可以有多个run同时发布，koala选择不同的版本
__.PKG包含编译器和虚拟机版本号

## 类型系统

## 类，封装，继承和多态
1. 类继承支持单继承
2. 接口不支持继承
3. obj, ok = typeof(object, class) or typeof(object)返回类型
4. var type Class = typeof(obj);
5. 向上转换，向下转换
6. super, self
7. obj, _ = type(v, class);

## QA
  xml封装
  编译解析xml文件??

## klc结构
index | type        | name      | attribute
  0      1,var          1           2
  1      2,func         3           4
  2      3,struct     index(Hello)
         4,intf
         5,field      index      Hello, 2
         6,method     index      Hello, 4
constant:

1  (
2  )
3  (V)
4 hello
5 koala/lang.string
6 SayHello
7 (koala/lang.string;)(V)
8 100
  1 5 ; 2 3

var_desc:
 name_index
 descriptor_index

field_desc:
 name_index
 descriptor_index
 struct_name_index

func_desc:
 name_index
 descriptor_index
 code_index
 code_length

method_desc:
 name_index
 descriptor_index
 struct_name_index
 code_index

struct_desc:
 name_index

code_desc:
 stack_size
 code_index
 code_length

code_area:
 xxx
 yyy
 zzz

handle SIGUSR1 nostop noprint

package一样，路径不同，可以访问内部符号吗？
包相同，导入路径不同，不能访问内部符号

git remote add origin git@github.com:zhuguangxiang/koala-language.git
git push -u origin master --tags
git tag -a v0.5.2 -m 'v0.5.2 release'
git commit --amend

export LD_LIBRARY_PATH="/home/zgx/koala-lang"

find . -name "*.[ch]" | xargs cat | wc -l

find . -name "*.[ch]"| xargs cat | grep -v ^$ | wc -l

valgrind --leak-check=full --show-leak-kinds=all ./a.out

Steps for compiler
1. build AST, variable declaration list is seperated into many single ones.
2. semantic analysis, all expressions' types are analysed and set if it's empty.
3. optimization expression
4. code generation

#### Version Log:
v0.5.1:
1. vector.c
2. add listobject.c
3. parse.c divides add symbols, type check, code gen three parts.
'add symbols' is in building AST.
4. koalac test-0.5.1.kl & koala test-0.5.1.klc OK!!!

v0.5.5:
1. bugfix: control flow: if-else statement
2. new: io.Println support
3. new: nested if-else statements and var decl in nested blocks ok

v0.5.6:
1. feature: while-stmt, do-while-stmt

v0.5.7:
1. feature: break, countinue statement

v0.5.8:
1. feature: class statement
2. function in class can access symbol in module, if there is the same
symbol in class and module, please use MODULE.SYMBOL to access module's symbol
3. run module-name, not module's file name,  module-name is trimmed ".klc"

v0.5.9:
1. feature: external module's access and management
    1). installed path:
    2). searching path: relative path & absolute path
2. feature: koala arguments:
    1). -MMAX = 100M
    2). -KPATH = /home/user/krepo, default:~/.koala-repo
    3). -Config = ~/koala.properties
3. feature: koalac arguments:
    1). -O0, -O1, -O2, -On
    2). -KOALAPATH
4. feature: properties file, argument format

v0.5.10:
1. feature: object inheritance, super keyword
2. feature: object memory model
3. feature: object init method, every object has default init func, whatever
there is an defined init func with different arguments. subclass no need call
super's init.
4. new: func call arguments check
5. subclass includes parent's symbols using namespace class name
6. fields and methods are not copied to subclass, and only fields memory is
allocated in subobject.
7. xxx function will be searched in super class, except __init__ function.
8. if field is with classname, then search the field from classname's class
9. if field is not with classname, then it can be serached in super class.
10. test-0.5.11.kl ok

v0.5.10-1:

v0.5.10-2:
1. feature: interface statement
2. test:test-0.5.13.kl

v0.5.10-3:
1. interface: support inherited
2. test:test-0.5.14.kl test:test-0.5.15.kl

v0.6.0:
1. feature: subclass casts to base class, and base class casts to realclass
2. keyword: typeof
   usage:
   typeof(variable-name or object) returns a type
   typeof(variable-name or object, type-name) is type cast to type-name
3. test-0.6.0.kl
4. typeof: call lang.TypeOf(...A) function
5. cast: interface <- interface, interface <- class,
    class <- class(up & down casts), class <- interface(up & down casts)

v0.6.1:
1. feature: reflect module

v0.6.2:
1. feature: error handle
2. bugfix: multi-value returns, e.g.: var a, b = fn(1, 2)
3. new: assignment type check
4. typealias: only allowed alias local package's type, not allowed external
package's type

v0.6.4:
1. feature: access control

v0.6.5:
1. feature: for statement

v0.6.6:
1. feature: break n, continue n statement or break LABEL?

v0.6.7:
1. refactor: symbol table

v0.6.8:
1. reactor: string object

v0.6.9:
1. feature: array object

v0.7.0:
1. refactor: klass's operations

v1.0.1:
1. refactor: type system, maybe add char? or byte?
2. bugfix: FIXME???
3. feature: coroutine, mutex access
4. feature: gc
5. feature: switch statement
6. feature: go statement
7. refactor: parser_ident()
8. write paper about koala language
9. VSCode plugin for koala
10. koala debugger and shell
11. optimization
12. like maven??? kar management
13. standard library
100. ... ...
