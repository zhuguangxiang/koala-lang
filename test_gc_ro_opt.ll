; ModuleID = 'test_gc_ro.ll'
source_filename = "test_gc"

define i8 @foo(i8 addrspace(1)* %0) #0 gc "statepoint-example" {
entry:
  %v1a = getelementptr i8, i8 addrspace(1)* %0, i32 0
  %v1 = load i8, i8 addrspace(1)* %v1a, align 1
  %statepoint_token = call token (i64, i32, void ()*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 2882400000, i32 0, void ()* @bar, i32 0, i32 0, i32 0, i32 0)
  ret i8 %v1
}

declare void @bar() #0

declare token @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 immarg, i32 immarg, void ()*, i32 immarg, i32 immarg, ...)

attributes #0 = { "frame-pointer"="all" }
