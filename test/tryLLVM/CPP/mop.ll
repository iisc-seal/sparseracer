; ModuleID = 'mop.cpp'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [47 x i8] c"%p of type %s (typesize %d) bytes freed at %s\0A\00", align 1
@.str1 = private unnamed_addr constant [40 x i8] c"%p of type %s allocated %d bytes at %s\0A\00", align 1

; Function Attrs: uwtable
define void @_Z10mopDeallociiPcS_(i32 %address, i32 %typeSize, i8* %type, i8* %debugLoc) #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i8*, align 8
  %4 = alloca i8*, align 8
  store i32 %address, i32* %1, align 4
  store i32 %typeSize, i32* %2, align 4
  store i8* %type, i8** %3, align 8
  store i8* %debugLoc, i8** %4, align 8
  %5 = load i32* %1, align 4
  %6 = load i8** %3, align 8
  %7 = load i32* %2, align 4
  %8 = load i8** %4, align 8
  %9 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([47 x i8]* @.str, i32 0, i32 0), i32 %5, i8* %6, i32 %7, i8* %8)
  ret void
}

declare i32 @printf(i8*, ...) #1

; Function Attrs: uwtable
define void @_Z8mopAllociiPcS_(i32 %address, i32 %memsize, i8* %type, i8* %debugLoc) #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i8*, align 8
  %4 = alloca i8*, align 8
  store i32 %address, i32* %1, align 4
  store i32 %memsize, i32* %2, align 4
  store i8* %type, i8** %3, align 8
  store i8* %debugLoc, i8** %4, align 8
  %5 = load i32* %1, align 4
  %6 = load i8** %3, align 8
  %7 = load i32* %2, align 4
  %8 = load i8** %4, align 8
  %9 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([40 x i8]* @.str1, i32 0, i32 0), i32 %5, i8* %6, i32 %7, i8* %8)
  ret void
}

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.5.0 "}
