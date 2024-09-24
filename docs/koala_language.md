# Koala Language Manual
Koala language is personal hobby developed language. It is a static typed language with modern features.

## Hello World
```
func main(args [string]) {
	print('hello, world')
}
```
Using the below command to compile, it will geneate the suffix `klc` byte code file.
> koala -c hello.kl

This will generate the `hello.klc` file. There are two methods to run it.  One is that the source file can be executed directly, the other is that excuting the `klc` file.
- Executing the koala without any arguments, and input is `kl` source file.
> koala hello.kl
- Executing the koala without any arugments and input is `klc` byte code file.
> koala hello.klc
