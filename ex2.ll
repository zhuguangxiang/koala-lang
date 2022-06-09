
%struct.Foo = type { i8*, i32 }

@.str = private unnamed_addr constant [6 x i8] c"hello\00", align 1
@.str.1 = private unnamed_addr constant [13 x i8] c"Foo(%s, %d)\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.Foo, align 8
  store i32 0, i32* %1, align 4
  %3 = getelementptr inbounds %struct.Foo, %struct.Foo* %2, i32 0, i32 0
  store i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str, i64 0, i64 0), i8** %3, align 8
  %4 = getelementptr inbounds %struct.Foo, %struct.Foo* %2, i32 0, i32 1
  store i32 5, i32* %4, align 8
  %5 = getelementptr inbounds %struct.Foo, %struct.Foo* %2, i32 0, i32 0
  %6 = load i8*, i8** %5, align 8
  %7 = getelementptr inbounds %struct.Foo, %struct.Foo* %2, i32 0, i32 1
  %8 = load i32, i32* %7, align 8
  %9 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @.str.1, i64 0, i64 0), i8* %6, i32 %8)
  ret i32 0
}

declare dso_local i32 @printf(i8*, ...) #1
