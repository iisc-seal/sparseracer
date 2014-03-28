; ModuleID = 'struct3.instrument.bc'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.product = type { i32, i32, %"struct.product::node" }
%"struct.product::node" = type { i32, i32* }

@.str = private unnamed_addr constant [4 x i8] c"%p\0A\00", align 1
@0 = private unnamed_addr constant [17 x i8] c"%struct.product*\00"
@1 = private unnamed_addr constant [6 x i8] c"_Znwm\00"
@2 = private unnamed_addr constant [5 x i8] c"i32*\00"
@3 = private unnamed_addr constant [6 x i8] c"_Znwm\00"

; Function Attrs: uwtable
define i32 @main() #0 {
  %p = alloca %struct.product*, align 8
  %1 = call noalias i8* @_Znwm(i64 24) #3
  %2 = ptrtoint i8* %1 to i64
  call void @mopAlloc(i64 %2, i64 24, i8* getelementptr inbounds ([17 x i8]* @0, i32 0, i32 0), i8* getelementptr inbounds ([6 x i8]* @1, i32 0, i32 0))
  %3 = bitcast i8* %1 to %struct.product*
  store %struct.product* %3, %struct.product** %p, align 8
  %4 = load %struct.product** %p, align 8
  %5 = getelementptr inbounds %struct.product* %4, i32 0, i32 0
  store i32 10, i32* %5, align 4
  %6 = load %struct.product** %p, align 8
  %7 = getelementptr inbounds %struct.product* %6, i32 0, i32 2
  %8 = getelementptr inbounds %"struct.product::node"* %7, i32 0, i32 0
  store i32 10, i32* %8, align 4
  %9 = call noalias i8* @_Znwm(i64 4) #3
  %10 = ptrtoint i8* %9 to i64
  call void @mopAlloc(i64 %10, i64 4, i8* getelementptr inbounds ([5 x i8]* @2, i32 0, i32 0), i8* getelementptr inbounds ([6 x i8]* @3, i32 0, i32 0))
  %11 = bitcast i8* %9 to i32*
  %12 = load %struct.product** %p, align 8
  %13 = getelementptr inbounds %struct.product* %12, i32 0, i32 2
  %14 = getelementptr inbounds %"struct.product::node"* %13, i32 0, i32 1
  store i32* %11, i32** %14, align 8
  %15 = load %struct.product** %p, align 8
  %16 = getelementptr inbounds %struct.product* %15, i32 0, i32 2
  %17 = getelementptr inbounds %"struct.product::node"* %16, i32 0, i32 1
  %18 = load i32** %17, align 8
  %19 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @.str, i32 0, i32 0), i32* %18)
  %20 = load %struct.product** %p, align 8
  %21 = getelementptr inbounds %struct.product* %20, i32 0, i32 2
  %22 = getelementptr inbounds %"struct.product::node"* %21, i32 0, i32 1
  %23 = load i32** %22, align 8
  store i32 3, i32* %23, align 4
  ret i32 0
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #1

declare i32 @printf(i8*, ...) #2

declare void @mopInstrument(i64, i64, i8*, i8*)

declare void @mopAlloc(i64, i64, i8*, i8*)

declare void @mopDealloc(i64, i64, i8*, i8*)

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { builtin }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.5.0 "}
