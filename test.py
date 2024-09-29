
def hello():
    pass

a = [1, 2, 3, 4, 5, 6, 7, 8]

print(a[:5])        # prints [1, 2, 3, 4, 5]
print(a[2:])        # prints [3, 4, 5, 6, 7, 8]
print(a[2:5])       # prints [3, 4, 5]
print(a[2:7:2])     # prints [3, 5, 7]

b = a[:5]
b[0] = 100
print(b)
print(a)
