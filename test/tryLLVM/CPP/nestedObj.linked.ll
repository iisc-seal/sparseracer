; ModuleID = 'mop.ll'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%class.Cylinder = type { %class.Circle*, double }
%class.Circle = type { double }

@.str = private unnamed_addr constant [47 x i8] c"%p of type %s (typesize %d) bytes freed at %s\0A\00", align 1
@.str1 = private unnamed_addr constant [40 x i8] c"%p of type %s allocated %d bytes at %s\0A\00", align 1
@.str2 = private unnamed_addr constant [10 x i8] c"foo at %x\00", align 1
@.str13 = private unnamed_addr constant [11 x i8] c"base at %x\00", align 1
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
  %foo = alloca %class.Cylinder*, align 8
  %2 = alloca i8*
  %3 = alloca i32
  store i32 0, i32* %1
  call void @llvm.dbg.declare(metadata !{%class.Cylinder** %foo}, metadata !56), !dbg !58
  %4 = call noalias i8* @_Znwm(i64 16) #6, !dbg !58
  %5 = ptrtoint i8* %4 to i64, !dbg !58
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z8mopAllociiPcS_ to void (i64, i64, i8*, i8*)*)(i64 %5, i64 16, i8* getelementptr inbounds ([17 x i8]* @0, i32 0, i32 0), i8* getelementptr inbounds ([69 x i8]* @1, i32 0, i32 0)), !dbg !58
  %6 = bitcast i8* %4 to %class.Cylinder*, !dbg !58
  invoke void @_ZN8CylinderC2Edd(%class.Cylinder* %6, double 1.000000e+01, double 2.000000e+01)
          to label %7 unwind label %17, !dbg !58

; <label>:7                                       ; preds = %0
  store %class.Cylinder* %6, %class.Cylinder** %foo, align 8, !dbg !59
  %8 = load %class.Cylinder** %foo, align 8, !dbg !61
  %9 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([10 x i8]* @.str2, i32 0, i32 0), %class.Cylinder* %8), !dbg !61
  %10 = load %class.Cylinder** %foo, align 8, !dbg !62
  %11 = icmp eq %class.Cylinder* %10, null, !dbg !62
  br i1 %11, label %16, label %12, !dbg !62

; <label>:12                                      ; preds = %7
  invoke void @_ZN8CylinderD2Ev(%class.Cylinder* %10)
          to label %13 unwind label %22, !dbg !63

; <label>:13                                      ; preds = %12
  %14 = bitcast %class.Cylinder* %10 to i8*, !dbg !65
  %15 = ptrtoint %class.Cylinder* %10 to i64, !dbg !65
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z10mopDeallociiPcS_ to void (i64, i32, i8*, i8*)*)(i64 %15, i32 16, i8* getelementptr inbounds ([17 x i8]* @2, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @3, i32 0, i32 0)), !dbg !65
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
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z10mopDeallociiPcS_ to void (i64, i32, i8*, i8*)*)(i64 %21, i32 1, i8* getelementptr inbounds ([4 x i8]* @4, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @5, i32 0, i32 0)), !dbg !68
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
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z10mopDeallociiPcS_ to void (i64, i32, i8*, i8*)*)(i64 %27, i32 16, i8* getelementptr inbounds ([17 x i8]* @6, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @7, i32 0, i32 0)), !dbg !68
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
declare void @llvm.dbg.declare(metadata, metadata) #2

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #3

declare i32 @__gxx_personality_v0(...)

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #4

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
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z8mopAllociiPcS_ to void (i64, i64, i8*, i8*)*)(i64 %10, i64 8, i8* getelementptr inbounds ([15 x i8]* @8, i32 0, i32 0), i8* getelementptr inbounds ([69 x i8]* @9, i32 0, i32 0)), !dbg !77
  %11 = bitcast i8* %9 to %class.Circle*, !dbg !77
  %12 = load double* %2, align 8, !dbg !77
  invoke void @_ZN6CircleC2Ed(%class.Circle* %11, double %12)
          to label %13 unwind label %18, !dbg !77

; <label>:13                                      ; preds = %0
  %14 = getelementptr inbounds %class.Cylinder* %6, i32 0, i32 0, !dbg !79
  store %class.Circle* %11, %class.Circle** %14, align 8, !dbg !79
  %15 = getelementptr inbounds %class.Cylinder* %6, i32 0, i32 0, !dbg !81
  %16 = load %class.Circle** %15, align 8, !dbg !81
  %17 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([11 x i8]* @.str13, i32 0, i32 0), %class.Circle* %16), !dbg !81
  ret void, !dbg !82

; <label>:18                                      ; preds = %0
  %19 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup, !dbg !83
  %20 = extractvalue { i8*, i32 } %19, 0, !dbg !83
  store i8* %20, i8** %4, !dbg !83
  %21 = extractvalue { i8*, i32 } %19, 1, !dbg !83
  store i32 %21, i32* %5, !dbg !83
  %22 = ptrtoint i8* %9 to i64, !dbg !83
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z10mopDeallociiPcS_ to void (i64, i32, i8*, i8*)*)(i64 %22, i32 1, i8* getelementptr inbounds ([4 x i8]* @10, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @11, i32 0, i32 0)), !dbg !83
  call void @_ZdlPv(i8* %9) #7, !dbg !83
  br label %23, !dbg !83

; <label>:23                                      ; preds = %18
  %24 = load i8** %4, !dbg !84
  %25 = load i32* %5, !dbg !84
  %26 = insertvalue { i8*, i32 } undef, i8* %24, 0, !dbg !84
  %27 = insertvalue { i8*, i32 } %26, i32 %25, 1, !dbg !84
  resume { i8*, i32 } %27, !dbg !84
}

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
  call void bitcast (void (i32, i32, i8*, i8*)* @_Z10mopDeallociiPcS_ to void (i64, i32, i8*, i8*)*)(i64 %8, i32 8, i8* getelementptr inbounds ([15 x i8]* @12, i32 0, i32 0), i8* getelementptr inbounds ([70 x i8]* @13, i32 0, i32 0)), !dbg !90
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

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone }
attributes #3 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nobuiltin nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { builtin }
attributes #7 = { builtin nounwind }

!llvm.ident = !{!0, !0}
!llvm.dbg.cu = !{!1}
!llvm.module.flags = !{!54, !55}

!0 = metadata !{metadata !"clang version 3.5.0 "}
!1 = metadata !{i32 786449, metadata !2, i32 4, metadata !"clang version 3.5.0 ", i1 false, metadata !"", i32 0, metadata !3, metadata !4, metadata !36, metadata !3, metadata !45, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp] [DW_LANG_C_plus_plus]
!2 = metadata !{metadata !"nestedObj.cpp", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!3 = metadata !{}
!4 = metadata !{metadata !5, metadata !7, metadata !9, metadata !26}
!5 = metadata !{i32 786451, metadata !6, null, metadata !"_IO_FILE", i32 273, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS8_IO_FILE"} ; [ DW_TAG_structure_type ] [_IO_FILE] [line 273, size 0, align 0, offset 0] [decl] [from ]
!6 = metadata !{metadata !"/usr/include/libio.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!7 = metadata !{i32 786451, metadata !8, null, metadata !"", i32 22, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS9_G_fpos_t"} ; [ DW_TAG_structure_type ] [line 22, size 0, align 0, offset 0] [decl] [from ]
!8 = metadata !{metadata !"/usr/include/_G_config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!9 = metadata !{i32 786434, metadata !2, null, metadata !"Cylinder", i32 9, i64 128, i64 64, i32 0, i32 0, null, metadata !10, i32 0, null, null, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_class_type ] [Cylinder] [line 9, size 128, align 64, offset 0] [def] [from ]
!10 = metadata !{metadata !11, metadata !13, metadata !15, metadata !20, metadata !23}
!11 = metadata !{i32 786445, metadata !2, metadata !"_ZTS8Cylinder", metadata !"base", i32 10, i64 64, i64 64, i64 0, i32 1, metadata !12} ; [ DW_TAG_member ] [_ZTS8Cylinder] [base] [line 10, size 64, align 64, offset 0] [private] [from ]
!12 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS6Circle"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS6Circle]
!13 = metadata !{i32 786445, metadata !2, metadata !"_ZTS8Cylinder", metadata !"height", i32 11, i64 64, i64 64, i64 64, i32 1, metadata !14} ; [ DW_TAG_member ] [_ZTS8Cylinder] [height] [line 11, size 64, align 64, offset 64] [private] [from double]
!14 = metadata !{i32 786468, null, null, metadata !"double", i32 0, i64 64, i64 64, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [double] [line 0, size 64, align 64, offset 0, enc DW_ATE_float]
!15 = metadata !{i32 786478, metadata !2, metadata !"_ZTS8Cylinder", metadata !"Cylinder", metadata !"Cylinder", metadata !"", i32 13, metadata !16, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !19, i32 13} ; [ DW_TAG_subprogram ] [line 13] [Cylinder]
!16 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !17, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!17 = metadata !{null, metadata !18, metadata !14, metadata !14}
!18 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 1088, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS8Cylinder]
!19 = metadata !{i32 786468}
!20 = metadata !{i32 786478, metadata !2, metadata !"_ZTS8Cylinder", metadata !"volume", metadata !"volume", metadata !"_ZN8Cylinder6volumeEv", i32 17, metadata !21, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !19, i32 17} ; [ DW_TAG_subprogram ] [line 17] [volume]
!21 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !22, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!22 = metadata !{metadata !14, metadata !18}
!23 = metadata !{i32 786478, metadata !2, metadata !"_ZTS8Cylinder", metadata !"~Cylinder", metadata !"~Cylinder", metadata !"", i32 21, metadata !24, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !19, i32 21} ; [ DW_TAG_subprogram ] [line 21] [~Cylinder]
!24 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !25, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!25 = metadata !{null, metadata !18}
!26 = metadata !{i32 786434, metadata !2, null, metadata !"Circle", i32 2, i64 64, i64 64, i32 0, i32 0, null, metadata !27, i32 0, null, null, metadata !"_ZTS6Circle"} ; [ DW_TAG_class_type ] [Circle] [line 2, size 64, align 64, offset 0] [def] [from ]
!27 = metadata !{metadata !28, metadata !29, metadata !33}
!28 = metadata !{i32 786445, metadata !2, metadata !"_ZTS6Circle", metadata !"radius", i32 3, i64 64, i64 64, i64 0, i32 1, metadata !14} ; [ DW_TAG_member ] [_ZTS6Circle] [radius] [line 3, size 64, align 64, offset 0] [private] [from double]
!29 = metadata !{i32 786478, metadata !2, metadata !"_ZTS6Circle", metadata !"Circle", metadata !"Circle", metadata !"", i32 5, metadata !30, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !19, i32 5} ; [ DW_TAG_subprogram ] [line 5] [Circle]
!30 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !31, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!31 = metadata !{null, metadata !32, metadata !14}
!32 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 1088, metadata !"_ZTS6Circle"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS6Circle]
!33 = metadata !{i32 786478, metadata !2, metadata !"_ZTS6Circle", metadata !"area", metadata !"area", metadata !"_ZN6Circle4areaEv", i32 6, metadata !34, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !19, i32 6} ; [ DW_TAG_subprogram ] [line 6] [area]
!34 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !35, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!35 = metadata !{metadata !14, metadata !32}
!36 = metadata !{metadata !37, metadata !42, metadata !43, metadata !44}
!37 = metadata !{i32 786478, metadata !2, metadata !38, metadata !"main", metadata !"main", metadata !"", i32 26, metadata !39, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 ()* @main, null, null, metadata !3, i32 26} ; [ DW_TAG_subprogram ] [line 26] [def] [main]
!38 = metadata !{i32 786473, metadata !2}         ; [ DW_TAG_file_type ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!39 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !40, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!40 = metadata !{metadata !41}
!41 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!42 = metadata !{i32 786478, metadata !2, metadata !"_ZTS8Cylinder", metadata !"~Cylinder", metadata !"~Cylinder", metadata !"_ZN8CylinderD2Ev", i32 21, metadata !24, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Cylinder*)* @_ZN8CylinderD2Ev, null, metadata !23, metadata !3, i32 21} ; [ DW_TAG_subprogram ] [line 21] [def] [~Cylinder]
!43 = metadata !{i32 786478, metadata !2, metadata !"_ZTS8Cylinder", metadata !"Cylinder", metadata !"Cylinder", metadata !"_ZN8CylinderC2Edd", i32 13, metadata !16, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Cylinder*, double, double)* @_ZN8CylinderC2Edd, null, metadata !15, metadata !3, i32 13} ; [ DW_TAG_subprogram ] [line 13] [def] [Cylinder]
!44 = metadata !{i32 786478, metadata !2, metadata !"_ZTS6Circle", metadata !"Circle", metadata !"Circle", metadata !"_ZN6CircleC2Ed", i32 5, metadata !30, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Circle*, double)* @_ZN6CircleC2Ed, null, metadata !29, metadata !3, i32 5} ; [ DW_TAG_subprogram ] [line 5] [def] [Circle]
!45 = metadata !{metadata !46, metadata !51}
!46 = metadata !{i32 786440, metadata !47, metadata !49, i32 95} ; [ DW_TAG_imported_declaration ]
!47 = metadata !{i32 786489, metadata !48, null, metadata !"std", i32 178} ; [ DW_TAG_namespace ] [std] [line 178]
!48 = metadata !{metadata !"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/x86_64-linux-gnu/c++/4.8/bits/c++config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!49 = metadata !{i32 786454, metadata !50, null, metadata !"FILE", i32 49, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS8_IO_FILE"} ; [ DW_TAG_typedef ] [FILE] [line 49, size 0, align 0, offset 0] [from _ZTS8_IO_FILE]
!50 = metadata !{metadata !"/usr/include/stdio.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!51 = metadata !{i32 786440, metadata !47, metadata !52, i32 96} ; [ DW_TAG_imported_declaration ]
!52 = metadata !{i32 786454, metadata !50, null, metadata !"fpos_t", i32 111, i64 0, i64 0, i64 0, i32 0, metadata !53} ; [ DW_TAG_typedef ] [fpos_t] [line 111, size 0, align 0, offset 0] [from _G_fpos_t]
!53 = metadata !{i32 786454, metadata !8, null, metadata !"_G_fpos_t", i32 26, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS9_G_fpos_t"} ; [ DW_TAG_typedef ] [_G_fpos_t] [line 26, size 0, align 0, offset 0] [from _ZTS9_G_fpos_t]
!54 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!55 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!56 = metadata !{i32 786688, metadata !37, metadata !"foo", metadata !38, i32 27, metadata !57, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [foo] [line 27]
!57 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS8Cylinder]
!58 = metadata !{i32 27, i32 0, metadata !37, null}
!59 = metadata !{i32 27, i32 0, metadata !60, null}
!60 = metadata !{i32 786443, metadata !2, metadata !37, i32 27, i32 0, i32 1, i32 3} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!61 = metadata !{i32 28, i32 0, metadata !37, null}
!62 = metadata !{i32 29, i32 0, metadata !37, null}
!63 = metadata !{i32 29, i32 0, metadata !64, null}
!64 = metadata !{i32 786443, metadata !2, metadata !37, i32 29, i32 0, i32 1, i32 4} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!65 = metadata !{i32 29, i32 0, metadata !66, null}
!66 = metadata !{i32 786443, metadata !2, metadata !37, i32 29, i32 0, i32 2, i32 5} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!67 = metadata !{i32 31, i32 0, metadata !37, null}
!68 = metadata !{i32 32, i32 0, metadata !37, null}
!69 = metadata !{i32 32, i32 0, metadata !70, null}
!70 = metadata !{i32 786443, metadata !2, metadata !71, i32 32, i32 0, i32 2, i32 7} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!71 = metadata !{i32 786443, metadata !2, metadata !37, i32 32, i32 0, i32 1, i32 6} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!72 = metadata !{i32 786689, metadata !43, metadata !"this", null, i32 16777216, metadata !57, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!73 = metadata !{i32 0, i32 0, metadata !43, null}
!74 = metadata !{i32 786689, metadata !43, metadata !"r", metadata !38, i32 33554445, metadata !14, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [r] [line 13]
!75 = metadata !{i32 13, i32 0, metadata !43, null}
!76 = metadata !{i32 786689, metadata !43, metadata !"h", metadata !38, i32 50331661, metadata !14, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [h] [line 13]
!77 = metadata !{i32 14, i32 0, metadata !78, null}
!78 = metadata !{i32 786443, metadata !2, metadata !43, i32 13, i32 0, i32 0, i32 1} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!79 = metadata !{i32 14, i32 0, metadata !80, null}
!80 = metadata !{i32 786443, metadata !2, metadata !78, i32 14, i32 0, i32 1, i32 8} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!81 = metadata !{i32 15, i32 0, metadata !78, null}
!82 = metadata !{i32 16, i32 0, metadata !43, null}
!83 = metadata !{i32 16, i32 0, metadata !78, null}
!84 = metadata !{i32 16, i32 0, metadata !85, null}
!85 = metadata !{i32 786443, metadata !2, metadata !78, i32 16, i32 0, i32 1, i32 9} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!86 = metadata !{i32 786689, metadata !42, metadata !"this", null, i32 16777216, metadata !57, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!87 = metadata !{i32 0, i32 0, metadata !42, null}
!88 = metadata !{i32 22, i32 0, metadata !89, null}
!89 = metadata !{i32 786443, metadata !2, metadata !42, i32 21, i32 0, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!90 = metadata !{i32 22, i32 0, metadata !91, null}
!91 = metadata !{i32 786443, metadata !2, metadata !89, i32 22, i32 0, i32 1, i32 10} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!92 = metadata !{i32 23, i32 0, metadata !42, null}
!93 = metadata !{i32 786689, metadata !44, metadata !"this", null, i32 16777216, metadata !12, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!94 = metadata !{i32 0, i32 0, metadata !44, null}
!95 = metadata !{i32 786689, metadata !44, metadata !"r", metadata !38, i32 33554437, metadata !14, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [r] [line 5]
!96 = metadata !{i32 5, i32 0, metadata !44, null}
