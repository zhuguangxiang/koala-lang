package main

import "fmt"

type Point struct{ x, y int }

func main() {
	l := []*Point{}
	for i := 0; i < 1000000; i++ {
		l = append(l, &Point{i, i + 1})
	}
	sum := 0
	for i := 0; i < len(l); i++ {
		sum += l[i].x
	}
	fmt.Println(sum)
}
