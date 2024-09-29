package main

import "fmt"

func main() {
    fmt.Println("Hello, World!")

	var s []string
	fmt.Println(s, s == nil, len(s) == 0)

	s = make([]string, 3)
	fmt.Println(s, "len:", len(s), "cap:", cap(s))

	s[0] = "a"
    s[1] = "b"
    s[2] = "c"
	fmt.Println(s)

	l := s[2:]
	fmt.Println(l)

	l[0] = "abc"

	fmt.Println(l)
	fmt.Println(s)
}
