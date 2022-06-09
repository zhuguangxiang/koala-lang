; ModuleID = 'sum.ll'
source_filename = "my_module"

@foo = external thread_local global i32

declare i32 @bar()

define internal i32 @getFoo() {
  %1 = load i32, i32* @foo, align 4
  ret i32 %1
}

define i32 @sum(i32 %a, i32 %0) {
entry:
  %gg = call i32 @bar()
  %tmp = add i32 %a, %gg
  ret i32 %tmp
}
