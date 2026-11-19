package main

import "fmt"

// iterative equivalent of Koala's tail-recursive sum (Koala applies TCO)
func main() {
	n, acc := 100000000, 0
	for n > 0 {
		acc += n
		n--
	}
	fmt.Println(acc)
}
