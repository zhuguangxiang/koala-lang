function inner(m)
    if m < 2 then
        return m
    end
    return inner(m-1) + inner(m-2)
end

inner(40)
