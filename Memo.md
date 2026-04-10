
# Memo

- fused-jmp-opt
- a = n + 1, b = a + 2, c = b + 3, return c
- push -> lua pass arguments?
- import_gloabl & import_func
- call 4 bytes or 8 bytes?
- inline
- object type {}

(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/ack.klc
16381

real 0m2.207s
user 0m2.200s
sys 0m0.006s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/fib.klc
102334155

real 0m2.893s
user 0m2.890s
sys 0m0.003s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/sum.klc
5000000050000000

real 0m0.563s
user 0m0.562s
sys 0m0.001s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/sum.klc
5000000050000000

real 0m0.567s
user 0m0.565s
sys 0m0.002s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/sum.klc
5000000050000000

real 0m0.556s
user 0m0.554s
sys 0m0.001s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/fib.klc
102334155

real 0m2.851s
user 0m2.847s
sys 0m0.002s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/fib.klc
102334155

real 0m2.886s
user 0m2.884s
sys 0m0.002s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/fib.klc
102334155

real 0m2.845s
user 0m2.842s
sys 0m0.002s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/ack.klc
16381

real 0m2.155s
user 0m2.150s
sys 0m0.005s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/ack.klc
16381

real 0m2.216s
user 0m2.213s
sys 0m0.002s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/ack.klc
16381

real 0m2.206s
user 0m2.203s
sys 0m0.002s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/ack.klc
16381

real 0m2.199s
user 0m2.189s
sys 0m0.009s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/ack.klc
16381

real 0m2.171s
user 0m2.164s
sys 0m0.006s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/ack.klc
16381

real 0m2.209s
user 0m2.205s
sys 0m0.004s
(base) james@ThinkBook:~/Codes/koala-lang$ time koala bench/ack.klc
16381

real 0m2.197s
user 0m2.190s
sys 0m0.006s
(base) james@ThinkBook:~/Codes/koala-lang$ time lua ack.lua
Ack(3, 11) = 16381
elapsed: 2.183332 sec

real 0m2.187s
user 0m2.183s
sys 0m0.005s
