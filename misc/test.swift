
let a = 1
// OP_CONST_I64_1 0
let b = a + 2
// OP_INT_ADD_IMM8 1, 0, 2

let arr = [1,2,3]
// OP_CONST_LOAD 2, 0

print(arr[2])
// OP_SEQ_GET 3, 2, 2
// OP_PUSH 3
// OP_CALL 1, 0, 1
print(arr[1...3])

3 in arr
// OP_CONTAINS

class Object {
    func hash() Int {
        ...
    }

    func __eq__() Bool {
        ...
    }

    func str() String {
        ...
    }
}

trait Iterable[T] {
    func iter() Iterator[T]
}

trait Iterator[T] : Iterable[T] {
    func next() T?
}

class String : Iterable[String] {
    func iter() Iterator[String] {
        return StringIterator()
    }
}

class StringIterator : Iterator[String] {
    s String
    index Int

    func __init__(s String) {
        self.s = s
        self.index = 0
    }

    func iter() Iterator[String] {
        return self
    }

    func next() String? {
        if (index < s.length()) {
            return String(s + index)
        } else {
            return None
        }
    }
}

trait Mapping[K, V] {
    func __get_item__(key K) V
    func __set_item__(key K, val V) Bool
}
