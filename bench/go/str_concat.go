package main

import "fmt"

func main() {
	s := ""
	for i := 0; i < 20000; i++ {
		s += "x"
	}
	fmt.Println(len(s))
}
