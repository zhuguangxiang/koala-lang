; ModuleID = 'test_gc_all.o'
source_filename = "llvm-link"

define void @foo() gc "statepoint-example" {
entry:
  %v2 = call i8 addrspace(1)* @malloc(i32 100)
  store i8 10, i8 addrspace(1)* %v2, align 1
  %statepoint_token = call token (i64, i32, void ()*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 2882400000, i32 0, void ()* @foo2, i32 0, i32 0, i32 0, i32 0) [ "gc-live"(i8 addrspace(1)* %v2) ]
  %v2.relocated = call coldcc i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token %statepoint_token, i32 0, i32 0) ; (%v2, %v2)
  store i8 20, i8 addrspace(1)* %v2.relocated, align 1
  ret void
}

declare i8 addrspace(1)* @malloc(i32)

declare token @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 immarg, i32 immarg, void ()*, i32 immarg, i32 immarg, ...)

; Function Attrs: nounwind readnone
declare i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token, i32 immarg, i32 immarg) #0

define void @foo2() gc "statepoint-example" {
entry:
  %v2 = call i8 addrspace(1)* @malloc(i32 100)
  store i8 100, i8 addrspace(1)* %v2, align 1
  %statepoint_token = call token (i64, i32, void ()*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_isVoidf(i64 2882400000, i32 0, void ()* @bar, i32 0, i32 0, i32 0, i32 0) [ "gc-live"(i8 addrspace(1)* %v2) ]
  %v2.relocated = call coldcc i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token %statepoint_token, i32 0, i32 0) ; (%v2, %v2)
  store i8 120, i8 addrspace(1)* %v2.relocated, align 1
  ret void
}

declare void @bar()

attributes #0 = { nounwind readnone }
