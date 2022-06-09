
%struct.StackMap = type { i32, i32, i32 }

@__LLVM_StackMaps = external dso_local constant %struct.StackMap, align 4

declare dso_local i8 addrspace(1)* @dummy_func()

declare void @show_stackmap(%struct.StackMap*)

declare void @llvm.memset.p0i8.i32(i8* nocapture, i8, i32, i32, i1)

define i8 addrspace(1)* @test1(i8 addrspace(1)* %obj) gc "statepoint-example" {
  %gep = getelementptr i8, i8 addrspace(1)* %obj, i64 10
  store i8 100, i8 addrspace(1)* %gep
  %abc = call i8 addrspace(1)* @dummy_func()
  %p = getelementptr i8, i8 addrspace(1)* %gep, i64 -20000
  %1 = getelementptr i8, i8 addrspace(1)* %abc, i64 -20000
  store i8 100, i8 addrspace(1)* %gep

  %r = bitcast i8 addrspace(1)* %gep to i32 addrspace(1)*
  %r3 = ptrtoint i8 addrspace(1)* %gep to i64
  %r4 = inttoptr i64 %r3 to i32*
  store i32 200, i32 * %r4

  %ab = call i8 addrspace(1)* @dummy_func()
  call void @malloc2(i64 1024)
  %r2 = alloca i8
  call void @llvm.memset.p0i8.i32(i8* %r2, i8 0, i32 4, i32 4, i1 false)
  ret i8 addrspace(1)* %1
}

define dso_local void @main() #0 {
  %1 = alloca i8*, align 8
  %2 = call noalias i8 addrspace(1)* @malloc(i64 128) #3
  %3 = call i8 addrspace(1)* @test1(i8 addrspace(1)* %2)
  call void @show_stackmap(%struct.StackMap* @__LLVM_StackMaps)
  ret void
}
declare dso_local noalias i8 addrspace(1)* @malloc(i64) #1

; Function Attrs: nounwind
declare dso_local void @malloc2(i64) "unknown"
