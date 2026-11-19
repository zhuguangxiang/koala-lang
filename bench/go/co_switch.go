// Goroutine switch microbenchmark, the Go analogue of bench/co_switch.kl.
// Two goroutines ping-pong via runtime.Gosched on a single P (GOMAXPROCS=1),
// matching Koala's N:1 model. Run: go run bench/go/co_switch.go
// ns/switch = elapsed_ns / (2*N).

package main

import (
	"fmt"
	"runtime"
	"time"
)

const N = 2_000_000

func main() {
	runtime.GOMAXPROCS(1)
	done := make(chan bool)
	go func() {
		for i := 0; i < N; i++ {
			runtime.Gosched()
		}
		done <- true
	}()
	start := time.Now()
	for i := 0; i < N; i++ {
		runtime.Gosched()
	}
	elapsed := time.Since(start).Nanoseconds()
	<-done
	fmt.Println("iters", N)
	fmt.Println("elapsed_ns", elapsed)
	fmt.Println("ns_per_iter", elapsed/N)
	fmt.Println("ns_per_switch", elapsed/(2*N))
}
