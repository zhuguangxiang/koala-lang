import sys

count = int(sys.argv[1])
print("func first() int { return 7 }")
for i in range(count):
    print(f"func dummy{i}() int {{ return 0 }}")
print("func target() int { return 42 }")
print("let callback = target")
print("func main() { print(callback()) }")
