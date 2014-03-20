; ModuleID = 'mop.ll'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.node = type { i32, %struct.node* }

@.str = private unnamed_addr constant [47 x i8] c"%p of type %s (typesize %d) bytes freed at %s\0A\00", align 1
@.str1 = private unnamed_addr constant [40 x i8] c"%p of type %s allocated %d bytes at %s\0A\00", align 1
@.str2 = private unnamed_addr constant [12 x i8] c"head @ %p \0A\00", align 1
@.str13 = private unnamed_addr constant [9 x i8] c"a @ %p \0A\00", align 1
@.str24 = private unnamed_addr constant [13 x i8] c"array @ %p \0A\00", align 1
@0 = private unnamed_addr constant [14 x i8] c"%struct.node*\00"
@1 = private unnamed_addr constant [66 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp:10(malloc)\00"
@2 = private unnamed_addr constant [14 x i8] c"%struct.node*\00"
@3 = private unnamed_addr constant [64 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp:12(free)\00"
@4 = private unnamed_addr constant [5 x i8] c"i32*\00"
@5 = private unnamed_addr constant [65 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp:14(_Znwm)\00"
@6 = private unnamed_addr constant [5 x i8] c"i32*\00"
@7 = private unnamed_addr constant [66 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp:16(_ZdlPv)\00"
@8 = private unnamed_addr constant [5 x i8] c"i32*\00"
@9 = private unnamed_addr constant [65 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp:18(_Znam)\00"
@10 = private unnamed_addr constant [5 x i8] c"i32*\00"
@11 = private unnamed_addr constant [66 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp:20(_ZdaPv)\00"

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

; Function Attrs: uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %head = alloca %struct.node*, align 8
  %a = alloca i32*, align 8
  %array = alloca i32*, align 8
  store i32 0, i32* %1
  call void @llvm.dbg.declare(metadata !{%struct.node** %head}, metadata !52), !dbg !53
  %2 = call noalias i8* @malloc(i64 16) #6, !dbg !53
  %3 = ptrtoint i8* %2 to i64, !dbg !53
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z8mopAllociiPcS_ to void (i64, i64, i8*, i8*)*)(i64 %3, i64 16, i8* getelementptr inbounds ([14 x i8]* @0, i32 0, i32 0), i8* getelementptr inbounds ([66 x i8]* @1, i32 0, i32 0)), !dbg !53
  %4 = bitcast i8* %2 to %struct.node*, !dbg !53
  store %struct.node* %4, %struct.node** %head, align 8, !dbg !53
  %5 = load %struct.node** %head, align 8, !dbg !54
  %6 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([12 x i8]* @.str2, i32 0, i32 0), %struct.node* %5), !dbg !54
  %7 = load %struct.node** %head, align 8, !dbg !55
  %8 = bitcast %struct.node* %7 to i8*, !dbg !55
  %9 = ptrtoint %struct.node* %7 to i64, !dbg !55
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z10mopDeallociiPcS_ to void (i64, i32, i8*, i8*)*)(i64 %9, i32 16, i8* getelementptr inbounds ([14 x i8]* @2, i32 0, i32 0), i8* getelementptr inbounds ([64 x i8]* @3, i32 0, i32 0)), !dbg !55
  call void @free(i8* %8) #6, !dbg !55
  call void @llvm.dbg.declare(metadata !{i32** %a}, metadata !56), !dbg !58
  %10 = call noalias i8* @_Znwm(i64 4) #7, !dbg !58
  %11 = ptrtoint i8* %10 to i64, !dbg !58
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z8mopAllociiPcS_ to void (i64, i64, i8*, i8*)*)(i64 %11, i64 4, i8* getelementptr inbounds ([5 x i8]* @4, i32 0, i32 0), i8* getelementptr inbounds ([65 x i8]* @5, i32 0, i32 0)), !dbg !58
  %12 = bitcast i8* %10 to i32*, !dbg !58
  store i32* %12, i32** %a, align 8, !dbg !58
  %13 = load i32** %a, align 8, !dbg !59
  %14 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([9 x i8]* @.str13, i32 0, i32 0), i32* %13), !dbg !59
  %15 = load i32** %a, align 8, !dbg !60
  %16 = icmp eq i32* %15, null, !dbg !60
  br i1 %16, label %20, label %17, !dbg !60

; <label>:17                                      ; preds = %0
  %18 = bitcast i32* %15 to i8*, !dbg !61
  %19 = ptrtoint i32* %15 to i64, !dbg !61
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z10mopDeallociiPcS_ to void (i64, i32, i8*, i8*)*)(i64 %19, i32 4, i8* getelementptr inbounds ([5 x i8]* @6, i32 0, i32 0), i8* getelementptr inbounds ([66 x i8]* @7, i32 0, i32 0)), !dbg !61
  call void @_ZdlPv(i8* %18) #8, !dbg !61
  br label %20, !dbg !61

; <label>:20                                      ; preds = %17, %0
  call void @llvm.dbg.declare(metadata !{i32** %array}, metadata !63), !dbg !64
  %21 = call noalias i8* @_Znam(i64 40) #7, !dbg !64
  %22 = ptrtoint i8* %21 to i64, !dbg !64
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z8mopAllociiPcS_ to void (i64, i64, i8*, i8*)*)(i64 %22, i64 40, i8* getelementptr inbounds ([5 x i8]* @8, i32 0, i32 0), i8* getelementptr inbounds ([65 x i8]* @9, i32 0, i32 0)), !dbg !64
  %23 = bitcast i8* %21 to i32*, !dbg !64
  store i32* %23, i32** %array, align 8, !dbg !64
  %24 = load i32** %array, align 8, !dbg !65
  %25 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([13 x i8]* @.str24, i32 0, i32 0), i32* %24), !dbg !65
  %26 = load i32** %array, align 8, !dbg !66
  %27 = icmp eq i32* %26, null, !dbg !66
  br i1 %27, label %31, label %28, !dbg !66

; <label>:28                                      ; preds = %20
  %29 = bitcast i32* %26 to i8*, !dbg !67
  %30 = ptrtoint i32* %26 to i64, !dbg !67
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z10mopDeallociiPcS_ to void (i64, i32, i8*, i8*)*)(i64 %30, i32 4, i8* getelementptr inbounds ([5 x i8]* @10, i32 0, i32 0), i8* getelementptr inbounds ([66 x i8]* @11, i32 0, i32 0)), !dbg !67
  call void @_ZdaPv(i8* %29) #8, !dbg !67
  br label %31, !dbg !67

; <label>:31                                      ; preds = %28, %20
  %32 = load i32* %1, !dbg !69
  ret i32 %32, !dbg !69
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #2

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #3

; Function Attrs: nounwind
declare void @free(i8*) #3

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #4

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #5

; Function Attrs: nobuiltin
declare noalias i8* @_Znam(i64) #4

; Function Attrs: nobuiltin nounwind
declare void @_ZdaPv(i8*) #5

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nobuiltin nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { nounwind }
attributes #7 = { builtin }
attributes #8 = { builtin nounwind }

!llvm.ident = !{!0, !0}
!llvm.dbg.cu = !{!1}
!llvm.module.flags = !{!50, !51}

!0 = metadata !{metadata !"clang version 3.5.0 "}
!1 = metadata !{i32 786449, metadata !2, i32 4, metadata !"clang version 3.5.0 ", i1 false, metadata !"", i32 0, metadata !3, metadata !4, metadata !27, metadata !3, metadata !32, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp] [DW_LANG_C_plus_plus]
!2 = metadata !{metadata !"free.cpp", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Free"}
!3 = metadata !{}
!4 = metadata !{metadata !5, metadata !7, metadata !12, metadata !17, metadata !19, metadata !21}
!5 = metadata !{i32 786451, metadata !6, null, metadata !"", i32 98, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS5div_t"} ; [ DW_TAG_structure_type ] [line 98, size 0, align 0, offset 0] [decl] [from ]
!6 = metadata !{metadata !"/usr/include/stdlib.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Free"}
!7 = metadata !{i32 786451, metadata !6, null, metadata !"", i32 106, i64 128, i64 64, i32 0, i32 0, null, metadata !8, i32 0, null, null, metadata !"_ZTS6ldiv_t"} ; [ DW_TAG_structure_type ] [line 106, size 128, align 64, offset 0] [def] [from ]
!8 = metadata !{metadata !9, metadata !11}
!9 = metadata !{i32 786445, metadata !6, metadata !"_ZTS6ldiv_t", metadata !"quot", i32 108, i64 64, i64 64, i64 0, i32 0, metadata !10} ; [ DW_TAG_member ] [_ZTS6ldiv_t] [quot] [line 108, size 64, align 64, offset 0] [from long int]
!10 = metadata !{i32 786468, null, null, metadata !"long int", i32 0, i64 64, i64 64, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [long int] [line 0, size 64, align 64, offset 0, enc DW_ATE_signed]
!11 = metadata !{i32 786445, metadata !6, metadata !"_ZTS6ldiv_t", metadata !"rem", i32 109, i64 64, i64 64, i64 64, i32 0, metadata !10} ; [ DW_TAG_member ] [_ZTS6ldiv_t] [rem] [line 109, size 64, align 64, offset 64] [from long int]
!12 = metadata !{i32 786451, metadata !6, null, metadata !"", i32 118, i64 128, i64 64, i32 0, i32 0, null, metadata !13, i32 0, null, null, metadata !"_ZTS7lldiv_t"} ; [ DW_TAG_structure_type ] [line 118, size 128, align 64, offset 0] [def] [from ]
!13 = metadata !{metadata !14, metadata !16}
!14 = metadata !{i32 786445, metadata !6, metadata !"_ZTS7lldiv_t", metadata !"quot", i32 120, i64 64, i64 64, i64 0, i32 0, metadata !15} ; [ DW_TAG_member ] [_ZTS7lldiv_t] [quot] [line 120, size 64, align 64, offset 0] [from long long int]
!15 = metadata !{i32 786468, null, null, metadata !"long long int", i32 0, i64 64, i64 64, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [long long int] [line 0, size 64, align 64, offset 0, enc DW_ATE_signed]
!16 = metadata !{i32 786445, metadata !6, metadata !"_ZTS7lldiv_t", metadata !"rem", i32 121, i64 64, i64 64, i64 64, i32 0, metadata !15} ; [ DW_TAG_member ] [_ZTS7lldiv_t] [rem] [line 121, size 64, align 64, offset 64] [from long long int]
!17 = metadata !{i32 786451, metadata !18, null, metadata !"_IO_FILE", i32 273, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS8_IO_FILE"} ; [ DW_TAG_structure_type ] [_IO_FILE] [line 273, size 0, align 0, offset 0] [decl] [from ]
!18 = metadata !{metadata !"/usr/include/libio.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Free"}
!19 = metadata !{i32 786451, metadata !20, null, metadata !"", i32 22, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS9_G_fpos_t"} ; [ DW_TAG_structure_type ] [line 22, size 0, align 0, offset 0] [decl] [from ]
!20 = metadata !{metadata !"/usr/include/_G_config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Free"}
!21 = metadata !{i32 786451, metadata !2, null, metadata !"node", i32 4, i64 128, i64 64, i32 0, i32 0, null, metadata !22, i32 0, null, null, metadata !"_ZTS4node"} ; [ DW_TAG_structure_type ] [node] [line 4, size 128, align 64, offset 0] [def] [from ]
!22 = metadata !{metadata !23, metadata !25}
!23 = metadata !{i32 786445, metadata !2, metadata !"_ZTS4node", metadata !"data", i32 5, i64 32, i64 32, i64 0, i32 0, metadata !24} ; [ DW_TAG_member ] [_ZTS4node] [data] [line 5, size 32, align 32, offset 0] [from int]
!24 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!25 = metadata !{i32 786445, metadata !2, metadata !"_ZTS4node", metadata !"next", i32 6, i64 64, i64 64, i64 64, i32 0, metadata !26} ; [ DW_TAG_member ] [_ZTS4node] [next] [line 6, size 64, align 64, offset 64] [from ]
!26 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS4node"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS4node]
!27 = metadata !{metadata !28}
!28 = metadata !{i32 786478, metadata !2, metadata !29, metadata !"main", metadata !"main", metadata !"", i32 9, metadata !30, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 ()* @main, null, null, metadata !3, i32 9} ; [ DW_TAG_subprogram ] [line 9] [def] [main]
!29 = metadata !{i32 786473, metadata !2}         ; [ DW_TAG_file_type ] [/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp]
!30 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !31, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!31 = metadata !{metadata !24}
!32 = metadata !{metadata !33, metadata !37, metadata !39, metadata !43, metadata !44, metadata !47}
!33 = metadata !{i32 786440, metadata !34, metadata !36, i32 118} ; [ DW_TAG_imported_declaration ]
!34 = metadata !{i32 786489, metadata !35, null, metadata !"std", i32 178} ; [ DW_TAG_namespace ] [std] [line 178]
!35 = metadata !{metadata !"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/x86_64-linux-gnu/c++/4.8/bits/c++config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Free"}
!36 = metadata !{i32 786454, metadata !6, null, metadata !"div_t", i32 102, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS5div_t"} ; [ DW_TAG_typedef ] [div_t] [line 102, size 0, align 0, offset 0] [from _ZTS5div_t]
!37 = metadata !{i32 786440, metadata !34, metadata !38, i32 119} ; [ DW_TAG_imported_declaration ]
!38 = metadata !{i32 786454, metadata !6, null, metadata !"ldiv_t", i32 110, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS6ldiv_t"} ; [ DW_TAG_typedef ] [ldiv_t] [line 110, size 0, align 0, offset 0] [from _ZTS6ldiv_t]
!39 = metadata !{i32 786440, metadata !40, metadata !42, i32 201} ; [ DW_TAG_imported_declaration ]
!40 = metadata !{i32 786489, metadata !41, null, metadata !"__gnu_cxx", i32 196} ; [ DW_TAG_namespace ] [__gnu_cxx] [line 196]
!41 = metadata !{metadata !"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/c++/4.8/cstdlib", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Free"}
!42 = metadata !{i32 786454, metadata !6, null, metadata !"lldiv_t", i32 122, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS7lldiv_t"} ; [ DW_TAG_typedef ] [lldiv_t] [line 122, size 0, align 0, offset 0] [from _ZTS7lldiv_t]
!43 = metadata !{i32 786440, metadata !34, metadata !42, i32 241} ; [ DW_TAG_imported_declaration ]
!44 = metadata !{i32 786440, metadata !34, metadata !45, i32 95} ; [ DW_TAG_imported_declaration ]
!45 = metadata !{i32 786454, metadata !46, null, metadata !"FILE", i32 49, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS8_IO_FILE"} ; [ DW_TAG_typedef ] [FILE] [line 49, size 0, align 0, offset 0] [from _ZTS8_IO_FILE]
!46 = metadata !{metadata !"/usr/include/stdio.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/Free"}
!47 = metadata !{i32 786440, metadata !34, metadata !48, i32 96} ; [ DW_TAG_imported_declaration ]
!48 = metadata !{i32 786454, metadata !46, null, metadata !"fpos_t", i32 111, i64 0, i64 0, i64 0, i32 0, metadata !49} ; [ DW_TAG_typedef ] [fpos_t] [line 111, size 0, align 0, offset 0] [from _G_fpos_t]
!49 = metadata !{i32 786454, metadata !20, null, metadata !"_G_fpos_t", i32 26, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS9_G_fpos_t"} ; [ DW_TAG_typedef ] [_G_fpos_t] [line 26, size 0, align 0, offset 0] [from _ZTS9_G_fpos_t]
!50 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!51 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!52 = metadata !{i32 786688, metadata !28, metadata !"head", metadata !29, i32 10, metadata !26, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [head] [line 10]
!53 = metadata !{i32 10, i32 0, metadata !28, null}
!54 = metadata !{i32 11, i32 0, metadata !28, null}
!55 = metadata !{i32 12, i32 0, metadata !28, null}
!56 = metadata !{i32 786688, metadata !28, metadata !"a", metadata !29, i32 14, metadata !57, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [a] [line 14]
!57 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !24} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from int]
!58 = metadata !{i32 14, i32 0, metadata !28, null}
!59 = metadata !{i32 15, i32 0, metadata !28, null}
!60 = metadata !{i32 16, i32 0, metadata !28, null}
!61 = metadata !{i32 16, i32 0, metadata !62, null}
!62 = metadata !{i32 786443, metadata !2, metadata !28, i32 16, i32 0, i32 1, i32 0} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp]
!63 = metadata !{i32 786688, metadata !28, metadata !"array", metadata !29, i32 18, metadata !57, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [array] [line 18]
!64 = metadata !{i32 18, i32 0, metadata !28, null}
!65 = metadata !{i32 19, i32 0, metadata !28, null}
!66 = metadata !{i32 20, i32 0, metadata !28, null}
!67 = metadata !{i32 20, i32 0, metadata !68, null}
!68 = metadata !{i32 786443, metadata !2, metadata !28, i32 20, i32 0, i32 1, i32 1} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/Free/free.cpp]
!69 = metadata !{i32 21, i32 0, metadata !28, null}
