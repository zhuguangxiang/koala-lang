package main

import "fmt"

func main() {
	l := []int{}
	for i := 0; i < 2000000; i++ {
		l = append(l, i)
	}
	sum := 0
	for i := 0; i < len(l); i++ {
		sum += l[i]
	}
	fmt.Println(sum)
}
