; ModuleID = 'mop.ll'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.node = type { i32, %struct.node* }

@.str = private unnamed_addr constant [40 x i8] c"%x of type %s and size %d bytes freed \0A\00", align 1
@.str1 = private unnamed_addr constant [12 x i8] c"head @ %x \0A\00", align 1
@0 = private unnamed_addr constant [14 x i8] c"%struct.node*\00"

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

; Function Attrs: uwtable
define i32 @main() #0 {
  %head = alloca %struct.node*, align 8
  %1 = call noalias i8* @malloc(i64 16) #3
  %2 = bitcast i8* %1 to %struct.node*
  store %struct.node* %2, %struct.node** %head, align 8
  %3 = load %struct.node** %head, align 8
  %4 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([12 x i8]* @.str1, i32 0, i32 0), %struct.node* %3)
  %5 = load %struct.node** %head, align 8
  %6 = bitcast %struct.node* %5 to i8*
  %7 = ptrtoint %struct.node* %5 to i64
  call void bitcast (void (i32, i8*, i32)* @_Z3mopiPci to void (i64, i8*, i32)*)(i64 %7, i8* getelementptr inbounds ([14 x i8]* @0, i32 0, i32 0), i32 16)
  call void @free(i8* %6) #3
  ret i32 0
}

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #2

; Function Attrs: nounwind
declare void @free(i8*) #2

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0, !0}

!0 = metadata !{metadata !"clang version 3.5.0 "}
