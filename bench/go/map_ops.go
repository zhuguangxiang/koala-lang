package main

import "fmt"

func main() {
	m := make(map[int]int)
	for i := 0; i < 1000000; i++ {
		m[i] = i
	}
	sum := 0
	for i := 0; i < 1000000; i++ {
		sum += m[i]
	}
	fmt.Println(sum)
}
