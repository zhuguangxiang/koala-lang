
function fib(n)

   if (n <= 1) then
      return n;
   end

    return fib(n-1) + fib(n-2)
end

function test_fib_40()
    s1 = os.time()
    fib(40)
    s2 = os.time()

    s = os.difftime(s2, s1)
    print(s)
end

test_fib_40()
