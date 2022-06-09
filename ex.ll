define void @dummy_func() {
  ret void
}
define i8* @test1(i8 addrspace(1)* %obj) gc "statepoint-example" {
  %gep = getelementptr i8, i8 addrspace(1)* %obj, i64 20000
  call void @dummy_func()
  %p = getelementptr i8, i8 addrspace(1)* %gep, i64 -20000
  store i8 2, i8 addrspace(1)* %gep
  %ret = addrspacecast i8 addrspace(1)* %gep to i8*
  ret i8* %ret
}

define dso_local void @main() #0 {
  %1 = alloca i8*, align 8
  %2 = call noalias i8 addrspace(1)* @malloc(i64 8) #3
  %3 = call i8* @test1(i8 addrspace(1)* %2)
  ret void
}
declare dso_local noalias i8 addrspace(1)* @malloc(i64) #1
