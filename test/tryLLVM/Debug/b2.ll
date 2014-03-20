; ModuleID = 'b2.cpp'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.node = type { i32, %struct.node* }

; Function Attrs: uwtable
define i32 @main() #0 {
  %head = alloca %struct.node*, align 8
  call void @llvm.dbg.declare(metadata !{%struct.node** %head}, metadata !42), !dbg !43
  %1 = call noalias i8* @_Znwm(i64 16) #3, !dbg !43
  %2 = bitcast i8* %1 to %struct.node*, !dbg !43
  store %struct.node* %2, %struct.node** %head, align 8, !dbg !43
  %3 = load %struct.node** %head, align 8, !dbg !44
  %4 = getelementptr inbounds %struct.node* %3, i32 0, i32 0, !dbg !44
  store i32 5, i32* %4, align 4, !dbg !44
  %5 = load %struct.node** %head, align 8, !dbg !45
  %6 = getelementptr inbounds %struct.node* %5, i32 0, i32 1, !dbg !45
  store %struct.node* null, %struct.node** %6, align 8, !dbg !45
  ret i32 0, !dbg !46
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #2

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { builtin }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!39, !40}
!llvm.ident = !{!41}

!0 = metadata !{i32 786449, metadata !1, i32 4, metadata !"clang version 3.5.0 ", i1 false, metadata !"", i32 0, metadata !2, metadata !3, metadata !22, metadata !2, metadata !27, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/home/anirudh/software/llvm/test/tryLLVM/Debug/b2.cpp] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"b2.cpp", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Debug"}
!2 = metadata !{}
!3 = metadata !{metadata !4, metadata !6, metadata !11, metadata !16}
!4 = metadata !{i32 786451, metadata !5, null, metadata !"", i32 98, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS5div_t"} ; [ DW_TAG_structure_type ] [line 98, size 0, align 0, offset 0] [decl] [from ]
!5 = metadata !{metadata !"/usr/include/stdlib.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Debug"}
!6 = metadata !{i32 786451, metadata !5, null, metadata !"", i32 106, i64 128, i64 64, i32 0, i32 0, null, metadata !7, i32 0, null, null, metadata !"_ZTS6ldiv_t"} ; [ DW_TAG_structure_type ] [line 106, size 128, align 64, offset 0] [def] [from ]
!7 = metadata !{metadata !8, metadata !10}
!8 = metadata !{i32 786445, metadata !5, metadata !"_ZTS6ldiv_t", metadata !"quot", i32 108, i64 64, i64 64, i64 0, i32 0, metadata !9} ; [ DW_TAG_member ] [_ZTS6ldiv_t] [quot] [line 108, size 64, align 64, offset 0] [from long int]
!9 = metadata !{i32 786468, null, null, metadata !"long int", i32 0, i64 64, i64 64, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [long int] [line 0, size 64, align 64, offset 0, enc DW_ATE_signed]
!10 = metadata !{i32 786445, metadata !5, metadata !"_ZTS6ldiv_t", metadata !"rem", i32 109, i64 64, i64 64, i64 64, i32 0, metadata !9} ; [ DW_TAG_member ] [_ZTS6ldiv_t] [rem] [line 109, size 64, align 64, offset 64] [from long int]
!11 = metadata !{i32 786451, metadata !5, null, metadata !"", i32 118, i64 128, i64 64, i32 0, i32 0, null, metadata !12, i32 0, null, null, metadata !"_ZTS7lldiv_t"} ; [ DW_TAG_structure_type ] [line 118, size 128, align 64, offset 0] [def] [from ]
!12 = metadata !{metadata !13, metadata !15}
!13 = metadata !{i32 786445, metadata !5, metadata !"_ZTS7lldiv_t", metadata !"quot", i32 120, i64 64, i64 64, i64 0, i32 0, metadata !14} ; [ DW_TAG_member ] [_ZTS7lldiv_t] [quot] [line 120, size 64, align 64, offset 0] [from long long int]
!14 = metadata !{i32 786468, null, null, metadata !"long long int", i32 0, i64 64, i64 64, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [long long int] [line 0, size 64, align 64, offset 0, enc DW_ATE_signed]
!15 = metadata !{i32 786445, metadata !5, metadata !"_ZTS7lldiv_t", metadata !"rem", i32 121, i64 64, i64 64, i64 64, i32 0, metadata !14} ; [ DW_TAG_member ] [_ZTS7lldiv_t] [rem] [line 121, size 64, align 64, offset 64] [from long long int]
!16 = metadata !{i32 786451, metadata !1, null, metadata !"node", i32 3, i64 128, i64 64, i32 0, i32 0, null, metadata !17, i32 0, null, null, metadata !"_ZTS4node"} ; [ DW_TAG_structure_type ] [node] [line 3, size 128, align 64, offset 0] [def] [from ]
!17 = metadata !{metadata !18, metadata !20}
!18 = metadata !{i32 786445, metadata !1, metadata !"_ZTS4node", metadata !"data", i32 4, i64 32, i64 32, i64 0, i32 0, metadata !19} ; [ DW_TAG_member ] [_ZTS4node] [data] [line 4, size 32, align 32, offset 0] [from int]
!19 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!20 = metadata !{i32 786445, metadata !1, metadata !"_ZTS4node", metadata !"next", i32 5, i64 64, i64 64, i64 64, i32 0, metadata !21} ; [ DW_TAG_member ] [_ZTS4node] [next] [line 5, size 64, align 64, offset 64] [from ]
!21 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS4node"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS4node]
!22 = metadata !{metadata !23}
!23 = metadata !{i32 786478, metadata !1, metadata !24, metadata !"main", metadata !"main", metadata !"", i32 8, metadata !25, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 ()* @main, null, null, metadata !2, i32 8} ; [ DW_TAG_subprogram ] [line 8] [def] [main]
!24 = metadata !{i32 786473, metadata !1}         ; [ DW_TAG_file_type ] [/home/anirudh/software/llvm/test/tryLLVM/Debug/b2.cpp]
!25 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !26, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!26 = metadata !{metadata !19}
!27 = metadata !{metadata !28, metadata !32, metadata !34, metadata !38}
!28 = metadata !{i32 786440, metadata !29, metadata !31, i32 118} ; [ DW_TAG_imported_declaration ]
!29 = metadata !{i32 786489, metadata !30, null, metadata !"std", i32 178} ; [ DW_TAG_namespace ] [std] [line 178]
!30 = metadata !{metadata !"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/x86_64-linux-gnu/c++/4.8/bits/c++config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Debug"}
!31 = metadata !{i32 786454, metadata !5, null, metadata !"div_t", i32 102, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS5div_t"} ; [ DW_TAG_typedef ] [div_t] [line 102, size 0, align 0, offset 0] [from _ZTS5div_t]
!32 = metadata !{i32 786440, metadata !29, metadata !33, i32 119} ; [ DW_TAG_imported_declaration ]
!33 = metadata !{i32 786454, metadata !5, null, metadata !"ldiv_t", i32 110, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS6ldiv_t"} ; [ DW_TAG_typedef ] [ldiv_t] [line 110, size 0, align 0, offset 0] [from _ZTS6ldiv_t]
!34 = metadata !{i32 786440, metadata !35, metadata !37, i32 201} ; [ DW_TAG_imported_declaration ]
!35 = metadata !{i32 786489, metadata !36, null, metadata !"__gnu_cxx", i32 196} ; [ DW_TAG_namespace ] [__gnu_cxx] [line 196]
!36 = metadata !{metadata !"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/c++/4.8/cstdlib", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Debug"}
!37 = metadata !{i32 786454, metadata !5, null, metadata !"lldiv_t", i32 122, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS7lldiv_t"} ; [ DW_TAG_typedef ] [lldiv_t] [line 122, size 0, align 0, offset 0] [from _ZTS7lldiv_t]
!38 = metadata !{i32 786440, metadata !29, metadata !37, i32 241} ; [ DW_TAG_imported_declaration ]
!39 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!40 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!41 = metadata !{metadata !"clang version 3.5.0 "}
!42 = metadata !{i32 786688, metadata !23, metadata !"head", metadata !24, i32 9, metadata !21, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [head] [line 9]
!43 = metadata !{i32 9, i32 0, metadata !23, null}
!44 = metadata !{i32 10, i32 0, metadata !23, null}
!45 = metadata !{i32 11, i32 0, metadata !23, null}
!46 = metadata !{i32 12, i32 0, metadata !23, null}
