; ModuleID = 'mop.cpp'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [40 x i8] c"%x of type %s and size %d bytes freed \0A\00", align 1

; Function Attrs: uwtable
define void @_Z3mopiPci(i32 %address, i8* %type, i32 %size) #0 {
  %1 = alloca i32, align 4
  %2 = alloca i8*, align 8
  %3 = alloca i32, align 4
  store i32 %address, i32* %1, align 4
  store i8* %type, i8** %2, align 8
  store i32 %size, i32* %3, align 4
  %4 = load i32* %1, align 4
  %5 = load i8** %2, align 8
  %6 = load i32* %3, align 4
  %7 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([40 x i8]* @.str, i32 0, i32 0), i32 %4, i8* %5, i32 %6)
  ret void
}

declare i32 @printf(i8*, ...) #1

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.5.0 "}
