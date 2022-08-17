; ModuleID = 'test_gc'
source_filename = "test_gc"

define i8 @foo(i8 addrspace(1)* %0) gc "statepoint-example" {
entry:
  %v1a = getelementptr i8, i8 addrspace(1)* %0, i32 0
  %v1 = load i8, i8 addrspace(1)* %v1a, align 1
  call void @bar()
  ret i8 %v1
}

declare void @bar()
