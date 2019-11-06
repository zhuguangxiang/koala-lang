
# Koala

Koala is a fast, flexible and modularized modern programming language.

# Hello World

```shell
[james@host ~]$ koala
koala 0.9.1 (Oct 27 2019)
[GCC 8.3.0] on Linux/x86_64 5.0.0-32-generic
> "Hello, World"
Hello World
>
```

# Install

```shell
git clone https://github.com/zhuguangxiang/koala-lang.git
cd koala-lang/
mkdir build
cd build/
cmake ..
make
```

For GNU/Linux, needs `cmake`, `gcc`, `flex/bison`, for example, in Ubuntu:

```shell
sudo apt install build-essential
sudo apt install flex
sudo apt install bison
sudo apt install cmake

// set environment variables
echo 'export PATH="~/.local/bin:$PATH"' >> ~/.profile
```

For Apple/MacOS, needs `cmake`, `XCode`, `flex/bison`, in MacOS, using
[`homebrew`](https://brew.sh/) to install dependencies softwares.

```shell
// XCode installed from `Apple` app store. It is too large, be patient!

brew install cmake
brew install flex
brew install bison

// set environment variables
echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
echo 'export PATH="/usr/local/opt/flex/bin:$PATH"' >> ~/.bash_profile
echo 'export PATH="~/.local/bin:$PATH"' >> ~/.bash_profile
```

The program `koala` is at `build/src/koala`, you can copy it to other places.

```shell
// `make install` will install `koala` into '~/.local/bin'
// so needs set `~/.local/bin` into environment variable `PATH`
make install
```

# Examples And Manual

## Examples

```go
// define a string variable
greeting := "hello, world"
// define a int variable
var value = 100
// also define a string variable
var name string = "james"
// also define a int variable, but no initial value
var age int

// function: right bracket must be at line end.
func add(v1 int, v2 int) int {
  v1 + v2
}

// name: "james, Chu"
name += ", Chu"
// value: 300
value = add(100, 200)

// import "io" for print
import "io"
io.println(name)
io.println(value)
```

## Execution Result

```shell
[james@host ~]$ koala
koala 0.9.1 (Oct 27 2019)
[GCC 8.3.0] on Linux/x86_64 5.0.0-32-generic
> // define a string variable
> greeting := "hello, world"
> // define a int variable
> var value = 100
> // also define a string variable
> var name string = "james"
> // also define a int variable, but no initial value
> var age int
>
> // function: right bracket must be at line end.
> func add(v1 int, v2 int) int {
    v1 + v2
  }
>
> // name: "james, Chu"
> name += ", Chu"
> // value: 300
> value = add(100, 200)
>
> // import "io" for print
> import "io"
> io.println(name)
james, Chu
> io.println(value)
300
>
```

## More Examples

[Examples](https://github.com/zhuguangxiang/koala-lang/tree/master/examples) And [Tests](https://github.com/zhuguangxiang/koala-lang/blob/master/test.kl)

## Manual

[Koala Language Manual](https://github.com/zhuguangxiang/koala-lang/blob/master/docs/manual.md "Koala Language Manual")

# Feedback

You can add issues on github, or contact us by zhuguangxiang@gmail.com

If you are using `Tencent` QQ, please add QQ: 963606291
