// Idle-goroutine memory/count stress, the Go analogue of bench/co_idle.kl.
// Spawn N goroutines all blocked on one channel recv, single P. Peak RSS is
// the metric. Run: /usr/bin/time -v go run bench/go/co_idle.go  (scale N via sed)

package main

import (
	"fmt"
	"runtime"
	"time"
)

const N = 100000

func main() {
	runtime.GOMAXPROCS(1)
	ch := make(chan int)
	for i := 0; i < N; i++ {
		go func() { <-ch }()
	}
	fmt.Println("spawned", N)
	time.Sleep(200 * time.Millisecond)
	fmt.Println("parked", N)
}
