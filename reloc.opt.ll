; ModuleID = 'reloc.ll'
source_filename = "reloc.ll"

%struct.StackMap = type { i32, i32, i32 }

@__LLVM_StackMaps = external dso_local constant %struct.StackMap, align 4

declare dso_local i8 addrspace(1)* @dummy_func()

declare void @show_stackmap(%struct.StackMap*)

define i8 addrspace(1)* @test1(i8 addrspace(1)* %obj) gc "statepoint-example" {
  %gep = getelementptr i8, i8 addrspace(1)* %obj, i64 10
  store i8 100, i8 addrspace(1)* %gep, align 1
  %statepoint_token = call token (i64, i32, i8 addrspace(1)* ()*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_p1i8f(i64 2882400000, i32 0, i8 addrspace(1)* ()* @dummy_func, i32 0, i32 0, i32 0, i32 0) [ "gc-live"(i8 addrspace(1)* %obj) ]
  %abc2 = call i8 addrspace(1)* @llvm.experimental.gc.result.p1i8(token %statepoint_token)
  %obj.relocated = call coldcc i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token %statepoint_token, i32 0, i32 0) ; (%obj, %obj)
  %gep.remat = getelementptr i8, i8 addrspace(1)* %obj.relocated, i64 10
  %p = getelementptr i8, i8 addrspace(1)* %gep.remat, i64 -20000
  %1 = getelementptr i8, i8 addrspace(1)* %abc2, i64 -20000
  store i8 100, i8 addrspace(1)* %gep.remat, align 1
  %r = bitcast i8 addrspace(1)* %gep.remat to i32 addrspace(1)*
  %r3 = ptrtoint i8 addrspace(1)* %gep.remat to i64
  %r4 = inttoptr i64 %r3 to i32*
  store i32 200, i32* %r4, align 4
  %statepoint_token3 = call token (i64, i32, i8 addrspace(1)* ()*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_p1i8f(i64 2882400000, i32 0, i8 addrspace(1)* ()* @dummy_func, i32 0, i32 0, i32 0, i32 0) [ "gc-live"(i8 addrspace(1)* %abc2) ]
  %abc.relocated = call coldcc i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token %statepoint_token3, i32 0, i32 0) ; (%abc2, %abc2)
  %.remat = getelementptr i8, i8 addrspace(1)* %abc.relocated, i64 -20000
  %statepoint_token4 = call token (i64, i32, void (i64)*, i32, i32, ...) @llvm.experimental.gc.statepoint.p0f_isVoidi64f(i64 2882400000, i32 0, void (i64)* @malloc2, i32 1, i32 0, i64 1024, i32 0, i32 0) [ "gc-live"(i8 addrspace(1)* %abc.relocated) ]
  %abc.relocated5 = call coldcc i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token %statepoint_token4, i32 0, i32 0) ; (%abc.relocated, %abc.relocated)
  %.remat1 = getelementptr i8, i8 addrspace(1)* %abc.relocated5, i64 -20000
  %r2 = alloca i8, align 1
  call void @llvm.memset.p0i8.i32(i8* align 4 %r2, i8 0, i32 4, i1 false)
  ret i8 addrspace(1)* %.remat1
}

define dso_local void @main() {
  %1 = alloca i8*, align 8
  %2 = call i8 addrspace(1)* @malloc(i64 128)
  %3 = call i8 addrspace(1)* @test1(i8 addrspace(1)* %2)
  call void @show_stackmap(%struct.StackMap* @__LLVM_StackMaps)
  ret void
}

declare dso_local i8 addrspace(1)* @malloc(i64)

declare dso_local void @malloc2(i64) #0

; Function Attrs: argmemonly nounwind willreturn writeonly
declare void @llvm.memset.p0i8.i32(i8* nocapture writeonly, i8, i32, i1 immarg) #1

declare void @__tmp_use(...)

declare token @llvm.experimental.gc.statepoint.p0f_p1i8f(i64 immarg, i32 immarg, i8 addrspace(1)* ()*, i32 immarg, i32 immarg, ...)

; Function Attrs: nounwind readonly
declare i8 addrspace(1)* @llvm.experimental.gc.result.p1i8(token) #2

; Function Attrs: nounwind readonly
declare i8 addrspace(1)* @llvm.experimental.gc.relocate.p1i8(token, i32 immarg, i32 immarg) #2

declare token @llvm.experimental.gc.statepoint.p0f_isVoidi64f(i64 immarg, i32 immarg, void (i64)*, i32 immarg, i32 immarg, ...)

attributes #0 = { "unknown" }
attributes #1 = { argmemonly nounwind willreturn writeonly }
attributes #2 = { nounwind readonly }
