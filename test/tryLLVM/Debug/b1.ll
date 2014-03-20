; ModuleID = 'b1.cpp'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %ptr = alloca i32*, align 8
  store i32 0, i32* %1
  call void @llvm.dbg.declare(metadata !{i32* %a}, metadata !12), !dbg !13
  call void @llvm.dbg.declare(metadata !{i32* %b}, metadata !14), !dbg !13
  call void @llvm.dbg.declare(metadata !{i32* %c}, metadata !15), !dbg !13
  call void @llvm.dbg.declare(metadata !{i32** %ptr}, metadata !16), !dbg !18
  %2 = call noalias i8* @_Znam(i64 40) #4, !dbg !19
  %3 = bitcast i8* %2 to i32*, !dbg !19
  store i32* %3, i32** %ptr, align 8, !dbg !19
  %4 = load i32** %ptr, align 8, !dbg !20
  %5 = getelementptr inbounds i32* %4, i64 1, !dbg !20
  store i32 5, i32* %5, align 4, !dbg !20
  %6 = load i32** %ptr, align 8, !dbg !21
  %7 = icmp eq i32* %6, null, !dbg !21
  br i1 %7, label %10, label %8, !dbg !21

; <label>:8                                       ; preds = %0
  %9 = bitcast i32* %6 to i8*, !dbg !22
  call void @_ZdaPv(i8* %9) #5, !dbg !22
  br label %10, !dbg !22

; <label>:10                                      ; preds = %8, %0
  store i32 6, i32* %b, align 4, !dbg !24
  store i32 7, i32* %c, align 4, !dbg !24
  %11 = load i32* %b, align 4, !dbg !25
  %12 = load i32* %c, align 4, !dbg !25
  %13 = add nsw i32 %11, %12, !dbg !25
  store i32 %13, i32* %a, align 4, !dbg !25
  %14 = load i32* %1, !dbg !26
  ret i32 %14, !dbg !26
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

; Function Attrs: nobuiltin
declare noalias i8* @_Znam(i64) #2

; Function Attrs: nobuiltin nounwind
declare void @_ZdaPv(i8*) #3

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nobuiltin nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { builtin }
attributes #5 = { builtin nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!9, !10}
!llvm.ident = !{!11}

!0 = metadata !{i32 786449, metadata !1, i32 4, metadata !"clang version 3.5.0 ", i1 false, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/home/anirudh/software/llvm/test/tryLLVM/Debug/b1.cpp] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"b1.cpp", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Debug"}
!2 = metadata !{}
!3 = metadata !{metadata !4}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"main", metadata !"main", metadata !"", i32 1, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 ()* @main, null, null, metadata !2, i32 1} ; [ DW_TAG_subprogram ] [line 1] [def] [main]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [/home/anirudh/software/llvm/test/tryLLVM/Debug/b1.cpp]
!6 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !7, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{metadata !8}
!8 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!9 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!10 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!11 = metadata !{metadata !"clang version 3.5.0 "}
!12 = metadata !{i32 786688, metadata !4, metadata !"a", metadata !5, i32 2, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [a] [line 2]
!13 = metadata !{i32 2, i32 0, metadata !4, null}
!14 = metadata !{i32 786688, metadata !4, metadata !"b", metadata !5, i32 2, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [b] [line 2]
!15 = metadata !{i32 786688, metadata !4, metadata !"c", metadata !5, i32 2, metadata !8, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [c] [line 2]
!16 = metadata !{i32 786688, metadata !4, metadata !"ptr", metadata !5, i32 3, metadata !17, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [ptr] [line 3]
!17 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !8} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from int]
!18 = metadata !{i32 3, i32 0, metadata !4, null}
!19 = metadata !{i32 4, i32 0, metadata !4, null}
!20 = metadata !{i32 5, i32 0, metadata !4, null}
!21 = metadata !{i32 6, i32 0, metadata !4, null}
!22 = metadata !{i32 6, i32 0, metadata !23, null}
!23 = metadata !{i32 786443, metadata !1, metadata !4, i32 6, i32 0, i32 1, i32 0} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/Debug/b1.cpp]
!24 = metadata !{i32 7, i32 0, metadata !4, null}
!25 = metadata !{i32 8, i32 0, metadata !4, null} ; [ DW_TAG_imported_declaration ]
!26 = metadata !{i32 9, i32 0, metadata !4, null}
