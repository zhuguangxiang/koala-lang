local function fib(n)
    if n < 2 then
        return n
    end
    return fib(n - 1) + fib(n - 2)
end

local function main()
    local n = 40

    local start = os.clock()
    local result = fib(n)
    local elapsed = os.clock() - start

    print("fib(" .. n .. ") = " .. result)
    print(string.format("elapsed: %.6f sec", elapsed))
end

main()
