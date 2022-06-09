; ModuleID = 'test_gc'
source_filename = "test_gc"

define void @foo2() gc "statepoint-example" {
entry:
  %v2 = call i8 addrspace(1)* @malloc(i32 100)
  store i8 100, i8 addrspace(1)* %v2, align 1
  call void @bar()
  store i8 120, i8 addrspace(1)* %v2, align 1
  ret void
}

declare i8 addrspace(1)* @malloc(i32 %0)

declare void @bar()
