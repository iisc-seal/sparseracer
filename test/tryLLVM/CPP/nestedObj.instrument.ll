; ModuleID = '<stdin>'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%class.Cylinder = type { %class.Circle*, double }
%class.Circle = type { double }

@.str = private unnamed_addr constant [10 x i8] c"foo at %x\00", align 1
@.str1 = private unnamed_addr constant [11 x i8] c"base at %x\00", align 1
@0 = private unnamed_addr constant [17 x i8] c"%class.Cylinder*\00"
@1 = private unnamed_addr constant [69 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp:27(_Znwm)\00"
@2 = private unnamed_addr constant [17 x i8] c"%class.Cylinder*\00"
@3 = private unnamed_addr constant [70 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp:29(_ZdlPv)\00"
@4 = private unnamed_addr constant [4 x i8] c"i8*\00"
@5 = private unnamed_addr constant [70 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp:32(_ZdlPv)\00"
@6 = private unnamed_addr constant [17 x i8] c"%class.Cylinder*\00"
@7 = private unnamed_addr constant [70 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp:32(_ZdlPv)\00"
@8 = private unnamed_addr constant [15 x i8] c"%class.Circle*\00"
@9 = private unnamed_addr constant [69 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp:14(_Znwm)\00"
@10 = private unnamed_addr constant [4 x i8] c"i8*\00"
@11 = private unnamed_addr constant [70 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp:16(_ZdlPv)\00"
@12 = private unnamed_addr constant [15 x i8] c"%class.Circle*\00"
@13 = private unnamed_addr constant [70 x i8] c"/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp:22(_ZdlPv)\00"

; Function Attrs: uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %foo = alloca %class.Cylinder*, align 8
  %2 = alloca i8*
  %3 = alloca i32
  store i32 0, i32* %1
  call void @llvm.dbg.declare(metadata !{%class.Cylinder** %foo}, metadata !56), !dbg !58
  %4 = call noalias i8* @_Znwm(i64 16) #6, !dbg !58
  %5 = ptrtoint i8* %4 to i64, !dbg !58
  call void @_Z8mopAllociiPcS_(i64 %5, i64 16, i8* getelementptr inbounds ([17 x i8]* @0, i32 0, i32 0), i8* getelementptr inbounds ([69 x i8]* @1, i32 0, i32 0)), !dbg !58
  %6 = bitcast i8* %4 to %class.Cylinder*, !dbg !58
  invoke void @_ZN8CylinderC2Edd(%class.Cylinder* %6, double 1.000000e+01, double 2.000000e+01)
          to label %7 unwind label %17, !dbg !58

; <label>:7                                       ; preds = %0
  store %class.Cylinder* %6, %class.Cylinder** %foo, align 8, !dbg !59
  %8 = load %class.Cylinder** %foo, align 8, !dbg !61
  %9 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([10 x i8]* @.str, i32 0, i32 0), %class.Cylinder* %8), !dbg !61
  %10 = load %class.Cylinder** %foo, align 8, !dbg !62
  %11 = icmp eq %class.Cylinder* %10, null, !dbg !62
  br i1 %11, label %16, label %12, !dbg !62

; <label>:12                                      ; preds = %7
  invoke void @_ZN8CylinderD2Ev(%class.Cylinder* %10)
          to label %13 unwind label %22, !dbg !63

; <label>:13                                      ; preds = %12
  %14 = bitcast %class.Cylinder* %10 to i8*, !dbg !65
  %15 = ptrtoint %class.Cylinder* %10 to i64, !dbg !65
  call void @_Z10mopDeallociiPcS_(i64 %15, i32 16, i8* getelementptr inbounds ([17 x i8]* @2, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @3, i32 0, i32 0)), !dbg !65
  call void @_ZdlPv(i8* %14) #7, !dbg !65
  br label %16, !dbg !65

; <label>:16                                      ; preds = %13, %7
  ret i32 0, !dbg !67

; <label>:17                                      ; preds = %0
  %18 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup, !dbg !68
  %19 = extractvalue { i8*, i32 } %18, 0, !dbg !68
  store i8* %19, i8** %2, !dbg !68
  %20 = extractvalue { i8*, i32 } %18, 1, !dbg !68
  store i32 %20, i32* %3, !dbg !68
  %21 = ptrtoint i8* %4 to i64, !dbg !68
  call void @_Z10mopDeallociiPcS_(i64 %21, i32 1, i8* getelementptr inbounds ([4 x i8]* @4, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @5, i32 0, i32 0)), !dbg !68
  call void @_ZdlPv(i8* %4) #7, !dbg !68
  br label %28, !dbg !68

; <label>:22                                      ; preds = %12
  %23 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup, !dbg !68
  %24 = extractvalue { i8*, i32 } %23, 0, !dbg !68
  store i8* %24, i8** %2, !dbg !68
  %25 = extractvalue { i8*, i32 } %23, 1, !dbg !68
  store i32 %25, i32* %3, !dbg !68
  %26 = bitcast %class.Cylinder* %10 to i8*, !dbg !68
  %27 = ptrtoint %class.Cylinder* %10 to i64, !dbg !68
  call void @_Z10mopDeallociiPcS_(i64 %27, i32 16, i8* getelementptr inbounds ([17 x i8]* @6, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @7, i32 0, i32 0)), !dbg !68
  call void @_ZdlPv(i8* %26) #7, !dbg !68
  br label %28, !dbg !68

; <label>:28                                      ; preds = %22, %17
  %29 = load i8** %2, !dbg !69
  %30 = load i32* %3, !dbg !69
  %31 = insertvalue { i8*, i32 } undef, i8* %29, 0, !dbg !69
  %32 = insertvalue { i8*, i32 } %31, i32 %30, 1, !dbg !69
  resume { i8*, i32 } %32, !dbg !69
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #2

; Function Attrs: uwtable
define linkonce_odr void @_ZN8CylinderC2Edd(%class.Cylinder* %this, double %r, double %h) unnamed_addr #0 align 2 {
  %1 = alloca %class.Cylinder*, align 8
  %2 = alloca double, align 8
  %3 = alloca double, align 8
  %4 = alloca i8*
  %5 = alloca i32
  store %class.Cylinder* %this, %class.Cylinder** %1, align 8
  call void @llvm.dbg.declare(metadata !{%class.Cylinder** %1}, metadata !72), !dbg !73
  store double %r, double* %2, align 8
  call void @llvm.dbg.declare(metadata !{double* %2}, metadata !74), !dbg !75
  store double %h, double* %3, align 8
  call void @llvm.dbg.declare(metadata !{double* %3}, metadata !76), !dbg !75
  %6 = load %class.Cylinder** %1
  %7 = getelementptr inbounds %class.Cylinder* %6, i32 0, i32 1, !dbg !75
  %8 = load double* %3, align 8, !dbg !75
  store double %8, double* %7, align 8, !dbg !75
  %9 = call noalias i8* @_Znwm(i64 8) #6, !dbg !77
  %10 = ptrtoint i8* %9 to i64, !dbg !77
  call void @_Z8mopAllociiPcS_(i64 %10, i64 8, i8* getelementptr inbounds ([15 x i8]* @8, i32 0, i32 0), i8* getelementptr inbounds ([69 x i8]* @9, i32 0, i32 0)), !dbg !77
  %11 = bitcast i8* %9 to %class.Circle*, !dbg !77
  %12 = load double* %2, align 8, !dbg !77
  invoke void @_ZN6CircleC2Ed(%class.Circle* %11, double %12)
          to label %13 unwind label %18, !dbg !77

; <label>:13                                      ; preds = %0
  %14 = getelementptr inbounds %class.Cylinder* %6, i32 0, i32 0, !dbg !79
  store %class.Circle* %11, %class.Circle** %14, align 8, !dbg !79
  %15 = getelementptr inbounds %class.Cylinder* %6, i32 0, i32 0, !dbg !81
  %16 = load %class.Circle** %15, align 8, !dbg !81
  %17 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([11 x i8]* @.str1, i32 0, i32 0), %class.Circle* %16), !dbg !81
  ret void, !dbg !82

; <label>:18                                      ; preds = %0
  %19 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup, !dbg !83
  %20 = extractvalue { i8*, i32 } %19, 0, !dbg !83
  store i8* %20, i8** %4, !dbg !83
  %21 = extractvalue { i8*, i32 } %19, 1, !dbg !83
  store i32 %21, i32* %5, !dbg !83
  %22 = ptrtoint i8* %9 to i64, !dbg !83
  call void @_Z10mopDeallociiPcS_(i64 %22, i32 1, i8* getelementptr inbounds ([4 x i8]* @10, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @11, i32 0, i32 0)), !dbg !83
  call void @_ZdlPv(i8* %9) #7, !dbg !83
  br label %23, !dbg !83

; <label>:23                                      ; preds = %18
  %24 = load i8** %4, !dbg !84
  %25 = load i32* %5, !dbg !84
  %26 = insertvalue { i8*, i32 } undef, i8* %24, 0, !dbg !84
  %27 = insertvalue { i8*, i32 } %26, i32 %25, 1, !dbg !84
  resume { i8*, i32 } %27, !dbg !84
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #3

declare i32 @printf(i8*, ...) #4

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN8CylinderD2Ev(%class.Cylinder* %this) unnamed_addr #5 align 2 {
  %1 = alloca %class.Cylinder*, align 8
  store %class.Cylinder* %this, %class.Cylinder** %1, align 8
  call void @llvm.dbg.declare(metadata !{%class.Cylinder** %1}, metadata !86), !dbg !87
  %2 = load %class.Cylinder** %1
  %3 = getelementptr inbounds %class.Cylinder* %2, i32 0, i32 0, !dbg !88
  %4 = load %class.Circle** %3, align 8, !dbg !88
  %5 = icmp eq %class.Circle* %4, null, !dbg !88
  br i1 %5, label %9, label %6, !dbg !88

; <label>:6                                       ; preds = %0
  %7 = bitcast %class.Circle* %4 to i8*, !dbg !90
  %8 = ptrtoint %class.Circle* %4 to i64, !dbg !90
  call void @_Z10mopDeallociiPcS_(i64 %8, i32 8, i8* getelementptr inbounds ([15 x i8]* @12, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @13, i32 0, i32 0)), !dbg !90
  call void @_ZdlPv(i8* %7) #7, !dbg !90
  br label %9, !dbg !90

; <label>:9                                       ; preds = %6, %0
  ret void, !dbg !92
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN6CircleC2Ed(%class.Circle* %this, double %r) unnamed_addr #5 align 2 {
  %1 = alloca %class.Circle*, align 8
  %2 = alloca double, align 8
  store %class.Circle* %this, %class.Circle** %1, align 8
  call void @llvm.dbg.declare(metadata !{%class.Circle** %1}, metadata !93), !dbg !94
  store double %r, double* %2, align 8
  call void @llvm.dbg.declare(metadata !{double* %2}, metadata !95), !dbg !96
  %3 = load %class.Circle** %1
  %4 = getelementptr inbounds %class.Circle* %3, i32 0, i32 0, !dbg !96
  %5 = load double* %2, align 8, !dbg !96
  store double %5, double* %4, align 8, !dbg !96
  ret void, !dbg !96
}

declare void @_Z8mopAllociiPcS_(i64, i64, i8*, i8*)

declare void @_Z10mopDeallociiPcS_(i64, i32, i8*, i8*)

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nobuiltin nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { builtin }
attributes #7 = { builtin nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!53, !54}
!llvm.ident = !{!55}

!0 = metadata !{i32 786449, metadata !1, i32 4, metadata !"clang version 3.5.0 ", i1 false, metadata !"", i32 0, metadata !2, metadata !3, metadata !35, metadata !2, metadata !44, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"nestedObj.cpp", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!2 = metadata !{}
!3 = metadata !{metadata !4, metadata !6, metadata !8, metadata !25}
!4 = metadata !{i32 786451, metadata !5, null, metadata !"_IO_FILE", i32 273, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS8_IO_FILE"} ; [ DW_TAG_structure_type ] [_IO_FILE] [line 273, size 0, align 0, offset 0] [decl] [from ]
!5 = metadata !{metadata !"/usr/include/libio.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!6 = metadata !{i32 786451, metadata !7, null, metadata !"", i32 22, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS9_G_fpos_t"} ; [ DW_TAG_structure_type ] [line 22, size 0, align 0, offset 0] [decl] [from ]
!7 = metadata !{metadata !"/usr/include/_G_config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!8 = metadata !{i32 786434, metadata !1, null, metadata !"Cylinder", i32 9, i64 128, i64 64, i32 0, i32 0, null, metadata !9, i32 0, null, null, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_class_type ] [Cylinder] [line 9, size 128, align 64, offset 0] [def] [from ]
!9 = metadata !{metadata !10, metadata !12, metadata !14, metadata !19, metadata !22}
!10 = metadata !{i32 786445, metadata !1, metadata !"_ZTS8Cylinder", metadata !"base", i32 10, i64 64, i64 64, i64 0, i32 1, metadata !11} ; [ DW_TAG_member ] [_ZTS8Cylinder] [base] [line 10, size 64, align 64, offset 0] [private] [from ]
!11 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS6Circle"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS6Circle]
!12 = metadata !{i32 786445, metadata !1, metadata !"_ZTS8Cylinder", metadata !"height", i32 11, i64 64, i64 64, i64 64, i32 1, metadata !13} ; [ DW_TAG_member ] [_ZTS8Cylinder] [height] [line 11, size 64, align 64, offset 64] [private] [from double]
!13 = metadata !{i32 786468, null, null, metadata !"double", i32 0, i64 64, i64 64, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [double] [line 0, size 64, align 64, offset 0, enc DW_ATE_float]
!14 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"Cylinder", metadata !"Cylinder", metadata !"", i32 13, metadata !15, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !18, i32 13} ; [ DW_TAG_subprogram ] [line 13] [Cylinder]
!15 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !16, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!16 = metadata !{null, metadata !17, metadata !13, metadata !13}
!17 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 1088, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS8Cylinder]
!18 = metadata !{i32 786468}
!19 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"volume", metadata !"volume", metadata !"_ZN8Cylinder6volumeEv", i32 17, metadata !20, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !18, i32 17} ; [ DW_TAG_subprogram ] [line 17] [volume]
!20 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !21, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!21 = metadata !{metadata !13, metadata !17}
!22 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"~Cylinder", metadata !"~Cylinder", metadata !"", i32 21, metadata !23, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !18, i32 21} ; [ DW_TAG_subprogram ] [line 21] [~Cylinder]
!23 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !24, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!24 = metadata !{null, metadata !17}
!25 = metadata !{i32 786434, metadata !1, null, metadata !"Circle", i32 2, i64 64, i64 64, i32 0, i32 0, null, metadata !26, i32 0, null, null, metadata !"_ZTS6Circle"} ; [ DW_TAG_class_type ] [Circle] [line 2, size 64, align 64, offset 0] [def] [from ]
!26 = metadata !{metadata !27, metadata !28, metadata !32}
!27 = metadata !{i32 786445, metadata !1, metadata !"_ZTS6Circle", metadata !"radius", i32 3, i64 64, i64 64, i64 0, i32 1, metadata !13} ; [ DW_TAG_member ] [_ZTS6Circle] [radius] [line 3, size 64, align 64, offset 0] [private] [from double]
!28 = metadata !{i32 786478, metadata !1, metadata !"_ZTS6Circle", metadata !"Circle", metadata !"Circle", metadata !"", i32 5, metadata !29, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !18, i32 5} ; [ DW_TAG_subprogram ] [line 5] [Circle]
!29 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !30, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!30 = metadata !{null, metadata !31, metadata !13}
!31 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 1088, metadata !"_ZTS6Circle"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS6Circle]
!32 = metadata !{i32 786478, metadata !1, metadata !"_ZTS6Circle", metadata !"area", metadata !"area", metadata !"_ZN6Circle4areaEv", i32 6, metadata !33, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !18, i32 6} ; [ DW_TAG_subprogram ] [line 6] [area]
!33 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !34, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!34 = metadata !{metadata !13, metadata !31}
!35 = metadata !{metadata !36, metadata !41, metadata !42, metadata !43}
!36 = metadata !{i32 786478, metadata !1, metadata !37, metadata !"main", metadata !"main", metadata !"", i32 26, metadata !38, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 ()* @main, null, null, metadata !2, i32 26} ; [ DW_TAG_subprogram ] [line 26] [def] [main]
!37 = metadata !{i32 786473, metadata !1}         ; [ DW_TAG_file_type ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!38 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !39, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!39 = metadata !{metadata !40}
!40 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!41 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"~Cylinder", metadata !"~Cylinder", metadata !"_ZN8CylinderD2Ev", i32 21, metadata !23, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Cylinder*)* @_ZN8CylinderD2Ev, null, metadata !22, metadata !2, i32 21} ; [ DW_TAG_subprogram ] [line 21] [def] [~Cylinder]
!42 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"Cylinder", metadata !"Cylinder", metadata !"_ZN8CylinderC2Edd", i32 13, metadata !15, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Cylinder*, double, double)* @_ZN8CylinderC2Edd, null, metadata !14, metadata !2, i32 13} ; [ DW_TAG_subprogram ] [line 13] [def] [Cylinder]
!43 = metadata !{i32 786478, metadata !1, metadata !"_ZTS6Circle", metadata !"Circle", metadata !"Circle", metadata !"_ZN6CircleC2Ed", i32 5, metadata !29, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Circle*, double)* @_ZN6CircleC2Ed, null, metadata !28, metadata !2, i32 5} ; [ DW_TAG_subprogram ] [line 5] [def] [Circle]
!44 = metadata !{metadata !45, metadata !50}
!45 = metadata !{i32 786440, metadata !46, metadata !48, i32 95} ; [ DW_TAG_imported_declaration ]
!46 = metadata !{i32 786489, metadata !47, null, metadata !"std", i32 178} ; [ DW_TAG_namespace ] [std] [line 178]
!47 = metadata !{metadata !"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/x86_64-linux-gnu/c++/4.8/bits/c++config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!48 = metadata !{i32 786454, metadata !49, null, metadata !"FILE", i32 49, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS8_IO_FILE"} ; [ DW_TAG_typedef ] [FILE] [line 49, size 0, align 0, offset 0] [from _ZTS8_IO_FILE]
!49 = metadata !{metadata !"/usr/include/stdio.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!50 = metadata !{i32 786440, metadata !46, metadata !51, i32 96} ; [ DW_TAG_imported_declaration ]
!51 = metadata !{i32 786454, metadata !49, null, metadata !"fpos_t", i32 111, i64 0, i64 0, i64 0, i32 0, metadata !52} ; [ DW_TAG_typedef ] [fpos_t] [line 111, size 0, align 0, offset 0] [from _G_fpos_t]
!52 = metadata !{i32 786454, metadata !7, null, metadata !"_G_fpos_t", i32 26, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS9_G_fpos_t"} ; [ DW_TAG_typedef ] [_G_fpos_t] [line 26, size 0, align 0, offset 0] [from _ZTS9_G_fpos_t]
!53 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!54 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!55 = metadata !{metadata !"clang version 3.5.0 "}
!56 = metadata !{i32 786688, metadata !36, metadata !"foo", metadata !37, i32 27, metadata !57, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [foo] [line 27]
!57 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS8Cylinder]
!58 = metadata !{i32 27, i32 0, metadata !36, null}
!59 = metadata !{i32 27, i32 0, metadata !60, null}
!60 = metadata !{i32 786443, metadata !1, metadata !36, i32 27, i32 0, i32 1, i32 3} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!61 = metadata !{i32 28, i32 0, metadata !36, null}
!62 = metadata !{i32 29, i32 0, metadata !36, null}
!63 = metadata !{i32 29, i32 0, metadata !64, null}
!64 = metadata !{i32 786443, metadata !1, metadata !36, i32 29, i32 0, i32 1, i32 4} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!65 = metadata !{i32 29, i32 0, metadata !66, null}
!66 = metadata !{i32 786443, metadata !1, metadata !36, i32 29, i32 0, i32 2, i32 5} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!67 = metadata !{i32 31, i32 0, metadata !36, null}
!68 = metadata !{i32 32, i32 0, metadata !36, null}
!69 = metadata !{i32 32, i32 0, metadata !70, null}
!70 = metadata !{i32 786443, metadata !1, metadata !71, i32 32, i32 0, i32 2, i32 7} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!71 = metadata !{i32 786443, metadata !1, metadata !36, i32 32, i32 0, i32 1, i32 6} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!72 = metadata !{i32 786689, metadata !42, metadata !"this", null, i32 16777216, metadata !57, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!73 = metadata !{i32 0, i32 0, metadata !42, null}
!74 = metadata !{i32 786689, metadata !42, metadata !"r", metadata !37, i32 33554445, metadata !13, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [r] [line 13]
!75 = metadata !{i32 13, i32 0, metadata !42, null}
!76 = metadata !{i32 786689, metadata !42, metadata !"h", metadata !37, i32 50331661, metadata !13, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [h] [line 13]
!77 = metadata !{i32 14, i32 0, metadata !78, null}
!78 = metadata !{i32 786443, metadata !1, metadata !42, i32 13, i32 0, i32 0, i32 1} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!79 = metadata !{i32 14, i32 0, metadata !80, null}
!80 = metadata !{i32 786443, metadata !1, metadata !78, i32 14, i32 0, i32 1, i32 8} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!81 = metadata !{i32 15, i32 0, metadata !78, null}
!82 = metadata !{i32 16, i32 0, metadata !42, null}
!83 = metadata !{i32 16, i32 0, metadata !78, null}
!84 = metadata !{i32 16, i32 0, metadata !85, null}
!85 = metadata !{i32 786443, metadata !1, metadata !78, i32 16, i32 0, i32 1, i32 9} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!86 = metadata !{i32 786689, metadata !41, metadata !"this", null, i32 16777216, metadata !57, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!87 = metadata !{i32 0, i32 0, metadata !41, null}
!88 = metadata !{i32 22, i32 0, metadata !89, null}
!89 = metadata !{i32 786443, metadata !1, metadata !41, i32 21, i32 0, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!90 = metadata !{i32 22, i32 0, metadata !91, null}
!91 = metadata !{i32 786443, metadata !1, metadata !89, i32 22, i32 0, i32 1, i32 10} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!92 = metadata !{i32 23, i32 0, metadata !41, null}
!93 = metadata !{i32 786689, metadata !43, metadata !"this", null, i32 16777216, metadata !11, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!94 = metadata !{i32 0, i32 0, metadata !43, null}
!95 = metadata !{i32 786689, metadata !43, metadata !"r", metadata !37, i32 33554437, metadata !13, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [r] [line 5]
!96 = metadata !{i32 5, i32 0, metadata !43, null}
