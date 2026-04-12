function shuffle(a, b, c, d, e)
    if a <= 0 then return e end
    return shuffle(a-1, e, d, c, b)
end

print(shuffle(40000000, 1, 2, 3, 4))
