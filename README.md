# What's Koala

Koala is a modern programming language. It is simple and extensible.

## Features of Koala

- Easy to use
- Object-oriented Programming (class, trait, mix-in)
- Functional Programming (closure)
- Type safe
- Garbage Collection

## Install

```shell
git clone https://github.com/zhuguangxiang/koala-lang.git
cd koala-lang/
mkdir build
cd build/
cmake ..
make
```

For GNU/Linux, needs `Cmake`, `gcc`, `flex/bison`, for example, in Ubuntu:

```shell
sudo apt install build-essential
sudo apt install flex
sudo apt install bison
sudo apt install cmake

# set environment variables
echo 'export PATH="~/.local/bin:$PATH"' >> ~/.profile
```

For Apple/MacOS, needs `Cmake`, `XCode`, `flex/bison`, in MacOS, using
[homebrew](https:brew.sh) to install dependencies softwares.

```shell
# XCode installed from `Apple` app store. It is too large, be patient!

brew install cmake
brew install flex
brew install bison

# set environment variables
echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
echo 'export PATH="/usr/local/opt/flex/bin:$PATH"' >> ~/.bash_profile
echo 'export PATH="~/.local/bin:$PATH"' >> ~/.bash_profile
```

The program `koala` is at 'build/src/koala', you can copy it to other places.

```shell
# `make install` will install `koala` into '~/.local/bin'
# so needs set `~/.local/bin` into environment variable `PATH`
make install
```

## Manual

[Koala Language Manual](https://github.com/zhuguangxiang/koala-lang/docs/manual.org)

## Home page

[www.koala-lang.org](https://www.koala-lang.org)

## The Author

Koala was originally designed and developed by [Zhu Guangxiang (James)](mailto:zhuguangxiang@gmail.com) since 2018.

## Donation

To support this project, you can make a donation to give me a cup of coffee 😃.
![WeChat Donation](./wechat-donation.png)
