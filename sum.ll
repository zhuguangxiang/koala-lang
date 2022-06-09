
%Foo = type { i8*, i32 }

@0 = private unnamed_addr constant [6 x i8] c"hello\00"
@1 = private unnamed_addr constant [14 x i8] c"Foo(%s, %d)\0A\00\00"


define i32* @std7Pointer1i6offset(i32* %0) {
  %2 = getelementptr inbounds i32, i32* %0, i32 3
  ret i32* %2
}

define internal i32 @Int8___add__(i32 *%self, i32 %rhs) {
  %1 = load i32, i32 *%self
  %2 = add i32 %1, %rhs
  ret i32 %2
}

define i32 @main() {
  %1 = alloca %Foo, align 8
  %2 = getelementptr inbounds %Foo, %Foo* %1, i32 0, i32 0
  %3 = load i8*, i8** %2, align 8
  store i8* getelementptr inbounds ([6 x i8], [6 x i8]* @0, i32 0, i32 0), i8** %2, align 8
  %4 = getelementptr inbounds %Foo, %Foo* %1, i32 0, i32 1
  %5 = load i32, i32* %4, align 4
  store i32 5, i32* %4, align 4
  %6 = load i8*, i8** %2, align 8
  %7 = load i32, i32* %4, align 4
  %8 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @1, i32 0, i32 0), i8* %6, i32 %7)
  %i = alloca i32
  store i32 5, i32* %i
  %r = load i32, i32* %i
  %9 = call i32 @Int8___add__(i32 *%4, i32 100)
  ret i32 %9
}

declare i32 @printf(i8* %0, ...)
