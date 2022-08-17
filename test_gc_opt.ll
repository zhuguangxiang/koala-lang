; ModuleID = 'test_gc.ll'
source_filename = "test_gc"

define void @foo() #0 gc "statepoint-example" {
entry:
  %v2 = call i8 addrspace(1)* @malloc(i32 100)
  store i8 10, i8 addrspace(1)* %v2, align 1
  %statepoint_token = call token (i64, i32, void ()*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 2882400000, i32 0, void ()* @bar, i32 0, i32 0, i32 0, i32 0) [ "gc-live"(i8 addrspace(1)* %v2) ]
  %v2.relocated = call coldcc i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token %statepoint_token, i32 0, i32 0) ; (%v2, %v2)
  store i8 20, i8 addrspace(1)* %v2.relocated, align 1
  ret void
}

declare i8 addrspace(1)* @malloc(i32) #0

declare void @bar() #0

declare void @__tmp_use(...)

declare token @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 immarg, i32 immarg, void ()*, i32 immarg, i32 immarg, ...)

; Function Attrs: nounwind readnone
declare i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token, i32 immarg, i32 immarg) #1

attributes #0 = { "frame-pointer"="all" }
attributes #1 = { nounwind readnone }
