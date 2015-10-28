; ModuleID = 'nestedObj.cpp'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%class.Cylinder = type { %class.Circle*, double }
%class.Circle = type { double }

@.str = private unnamed_addr constant [12 x i8] c"foo at %x \0A\00", align 1
@.str1 = private unnamed_addr constant [13 x i8] c"base at %x \0A\00", align 1

; Function Attrs: uwtable
define i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %foo = alloca %class.Cylinder*, align 8
  %exn.slot = alloca i8*
  %ehselector.slot = alloca i32
  store i32 0, i32* %retval
  call void @llvm.dbg.declare(metadata !{%class.Cylinder** %foo}, metadata !60), !dbg !62
  %call = call noalias i8* @_Znwm(i64 16) #6, !dbg !62
  %0 = bitcast i8* %call to %class.Cylinder*, !dbg !62
  invoke void @_ZN8CylinderC2Edd(%class.Cylinder* %0, double 1.000000e+01, double 2.000000e+01)
          to label %invoke.cont unwind label %lpad, !dbg !62

invoke.cont:                                      ; preds = %entry
  store %class.Cylinder* %0, %class.Cylinder** %foo, align 8, !dbg !63
  %1 = load %class.Cylinder** %foo, align 8, !dbg !65
  %call1 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([12 x i8]* @.str, i32 0, i32 0), %class.Cylinder* %1), !dbg !65
  %2 = load %class.Cylinder** %foo, align 8, !dbg !66
  %isnull = icmp eq %class.Cylinder* %2, null, !dbg !66
  br i1 %isnull, label %delete.end, label %delete.notnull, !dbg !66

delete.notnull:                                   ; preds = %invoke.cont
  invoke void @_ZN8CylinderD2Ev(%class.Cylinder* %2)
          to label %invoke.cont3 unwind label %lpad2, !dbg !67

invoke.cont3:                                     ; preds = %delete.notnull
  %3 = bitcast %class.Cylinder* %2 to i8*, !dbg !69
  call void @_ZdlPv(i8* %3) #7, !dbg !69
  br label %delete.end, !dbg !69

delete.end:                                       ; preds = %invoke.cont3, %invoke.cont
  ret i32 0, !dbg !71

lpad:                                             ; preds = %entry
  %4 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup, !dbg !72
  %5 = extractvalue { i8*, i32 } %4, 0, !dbg !72
  store i8* %5, i8** %exn.slot, !dbg !72
  %6 = extractvalue { i8*, i32 } %4, 1, !dbg !72
  store i32 %6, i32* %ehselector.slot, !dbg !72
  call void @_ZdlPv(i8* %call) #7, !dbg !72
  br label %eh.resume, !dbg !72

lpad2:                                            ; preds = %delete.notnull
  %7 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup, !dbg !72
  %8 = extractvalue { i8*, i32 } %7, 0, !dbg !72
  store i8* %8, i8** %exn.slot, !dbg !72
  %9 = extractvalue { i8*, i32 } %7, 1, !dbg !72
  store i32 %9, i32* %ehselector.slot, !dbg !72
  %10 = bitcast %class.Cylinder* %2 to i8*, !dbg !72
  call void @_ZdlPv(i8* %10) #7, !dbg !72
  br label %eh.resume, !dbg !72

eh.resume:                                        ; preds = %lpad2, %lpad
  %exn = load i8** %exn.slot, !dbg !73
  %sel = load i32* %ehselector.slot, !dbg !73
  %lpad.val = insertvalue { i8*, i32 } undef, i8* %exn, 0, !dbg !73
  %lpad.val4 = insertvalue { i8*, i32 } %lpad.val, i32 %sel, 1, !dbg !73
  resume { i8*, i32 } %lpad.val4, !dbg !73
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata) #1

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #2

; Function Attrs: uwtable
define linkonce_odr void @_ZN8CylinderC2Edd(%class.Cylinder* %this, double %r, double %h) unnamed_addr #0 align 2 {
entry:
  %this.addr = alloca %class.Cylinder*, align 8
  %r.addr = alloca double, align 8
  %h.addr = alloca double, align 8
  %exn.slot = alloca i8*
  %ehselector.slot = alloca i32
  store %class.Cylinder* %this, %class.Cylinder** %this.addr, align 8
  call void @llvm.dbg.declare(metadata !{%class.Cylinder** %this.addr}, metadata !76), !dbg !77
  store double %r, double* %r.addr, align 8
  call void @llvm.dbg.declare(metadata !{double* %r.addr}, metadata !78), !dbg !79
  store double %h, double* %h.addr, align 8
  call void @llvm.dbg.declare(metadata !{double* %h.addr}, metadata !80), !dbg !79
  %this1 = load %class.Cylinder** %this.addr
  %height = getelementptr inbounds %class.Cylinder* %this1, i32 0, i32 1, !dbg !79
  %0 = load double* %h.addr, align 8, !dbg !79
  store double %0, double* %height, align 8, !dbg !79
  %call = call noalias i8* @_Znwm(i64 8) #6, !dbg !81
  %1 = bitcast i8* %call to %class.Circle*, !dbg !81
  %2 = load double* %r.addr, align 8, !dbg !81
  invoke void @_ZN6CircleC2Ed(%class.Circle* %1, double %2)
          to label %invoke.cont unwind label %lpad, !dbg !81

invoke.cont:                                      ; preds = %entry
  %base = getelementptr inbounds %class.Cylinder* %this1, i32 0, i32 0, !dbg !83
  store %class.Circle* %1, %class.Circle** %base, align 8, !dbg !83
  %base2 = getelementptr inbounds %class.Cylinder* %this1, i32 0, i32 0, !dbg !85
  %3 = load %class.Circle** %base2, align 8, !dbg !85
  %call3 = call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([13 x i8]* @.str1, i32 0, i32 0), %class.Circle* %3), !dbg !85
  ret void, !dbg !86

lpad:                                             ; preds = %entry
  %4 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup, !dbg !87
  %5 = extractvalue { i8*, i32 } %4, 0, !dbg !87
  store i8* %5, i8** %exn.slot, !dbg !87
  %6 = extractvalue { i8*, i32 } %4, 1, !dbg !87
  store i32 %6, i32* %ehselector.slot, !dbg !87
  call void @_ZdlPv(i8* %call) #7, !dbg !87
  br label %eh.resume, !dbg !87

eh.resume:                                        ; preds = %lpad
  %exn = load i8** %exn.slot, !dbg !88
  %sel = load i32* %ehselector.slot, !dbg !88
  %lpad.val = insertvalue { i8*, i32 } undef, i8* %exn, 0, !dbg !88
  %lpad.val4 = insertvalue { i8*, i32 } %lpad.val, i32 %sel, 1, !dbg !88
  resume { i8*, i32 } %lpad.val4, !dbg !88
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #3

declare i32 @printf(i8*, ...) #4

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN8CylinderD2Ev(%class.Cylinder* %this) unnamed_addr #5 align 2 {
entry:
  %this.addr = alloca %class.Cylinder*, align 8
  store %class.Cylinder* %this, %class.Cylinder** %this.addr, align 8
  call void @llvm.dbg.declare(metadata !{%class.Cylinder** %this.addr}, metadata !90), !dbg !91
  %this1 = load %class.Cylinder** %this.addr
  %base = getelementptr inbounds %class.Cylinder* %this1, i32 0, i32 0, !dbg !92
  %0 = load %class.Circle** %base, align 8, !dbg !92
  %isnull = icmp eq %class.Circle* %0, null, !dbg !92
  br i1 %isnull, label %delete.end, label %delete.notnull, !dbg !92

delete.notnull:                                   ; preds = %entry
  %1 = bitcast %class.Circle* %0 to i8*, !dbg !94
  call void @_ZdlPv(i8* %1) #7, !dbg !94
  br label %delete.end, !dbg !94

delete.end:                                       ; preds = %delete.notnull, %entry
  ret void, !dbg !96
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN6CircleC2Ed(%class.Circle* %this, double %r) unnamed_addr #5 align 2 {
entry:
  %this.addr = alloca %class.Circle*, align 8
  %r.addr = alloca double, align 8
  store %class.Circle* %this, %class.Circle** %this.addr, align 8
  call void @llvm.dbg.declare(metadata !{%class.Circle** %this.addr}, metadata !97), !dbg !98
  store double %r, double* %r.addr, align 8
  call void @llvm.dbg.declare(metadata !{double* %r.addr}, metadata !99), !dbg !100
  %this1 = load %class.Circle** %this.addr
  %radius = getelementptr inbounds %class.Circle* %this1, i32 0, i32 0, !dbg !100
  %0 = load double* %r.addr, align 8, !dbg !100
  store double %0, double* %radius, align 8, !dbg !100
  ret void, !dbg !100
}

attributes #0 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nobuiltin nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { builtin }
attributes #7 = { builtin nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!57, !58}
!llvm.ident = !{!59}

!0 = metadata !{i32 786449, metadata !1, i32 4, metadata !"clang version 3.5.0 ", i1 false, metadata !"", i32 0, metadata !2, metadata !3, metadata !39, metadata !2, metadata !48, metadata !"", i32 1} ; [ DW_TAG_compile_unit ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"nestedObj.cpp", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!2 = metadata !{}
!3 = metadata !{metadata !4, metadata !6, metadata !8, metadata !27}
!4 = metadata !{i32 786451, metadata !5, null, metadata !"_IO_FILE", i32 273, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS8_IO_FILE"} ; [ DW_TAG_structure_type ] [_IO_FILE] [line 273, size 0, align 0, offset 0] [decl] [from ]
!5 = metadata !{metadata !"/usr/include/libio.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!6 = metadata !{i32 786451, metadata !7, null, metadata !"", i32 22, i64 0, i64 0, i32 0, i32 4, null, null, i32 0, null, null, metadata !"_ZTS9_G_fpos_t"} ; [ DW_TAG_structure_type ] [line 22, size 0, align 0, offset 0] [decl] [from ]
!7 = metadata !{metadata !"/usr/include/_G_config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!8 = metadata !{i32 786434, metadata !1, null, metadata !"Cylinder", i32 9, i64 128, i64 64, i32 0, i32 0, null, metadata !9, i32 0, null, null, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_class_type ] [Cylinder] [line 9, size 128, align 64, offset 0] [def] [from ]
!9 = metadata !{metadata !10, metadata !12, metadata !14, metadata !19, metadata !23}
!10 = metadata !{i32 786445, metadata !1, metadata !"_ZTS8Cylinder", metadata !"base", i32 10, i64 64, i64 64, i64 0, i32 1, metadata !11} ; [ DW_TAG_member ] [_ZTS8Cylinder] [base] [line 10, size 64, align 64, offset 0] [private] [from ]
!11 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS6Circle"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS6Circle]
!12 = metadata !{i32 786445, metadata !1, metadata !"_ZTS8Cylinder", metadata !"height", i32 11, i64 64, i64 64, i64 64, i32 1, metadata !13} ; [ DW_TAG_member ] [_ZTS8Cylinder] [height] [line 11, size 64, align 64, offset 64] [private] [from double]
!13 = metadata !{i32 786468, null, null, metadata !"double", i32 0, i64 64, i64 64, i64 0, i32 0, i32 4} ; [ DW_TAG_base_type ] [double] [line 0, size 64, align 64, offset 0, enc DW_ATE_float]
!14 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"Cylinder", metadata !"Cylinder", metadata !"", i32 13, metadata !15, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !18, i32 13} ; [ DW_TAG_subprogram ] [line 13] [Cylinder]
!15 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !16, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!16 = metadata !{null, metadata !17, metadata !13, metadata !13}
!17 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 1088, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS8Cylinder]
!18 = metadata !{i32 786468}
!19 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"volume", metadata !"volume", metadata !"_ZN8Cylinder6volumeEv", i32 17, metadata !20, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !22, i32 17} ; [ DW_TAG_subprogram ] [line 17] [volume]
!20 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !21, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!21 = metadata !{metadata !13, metadata !17}
!22 = metadata !{i32 786468}
!23 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"~Cylinder", metadata !"~Cylinder", metadata !"", i32 21, metadata !24, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !26, i32 21} ; [ DW_TAG_subprogram ] [line 21] [~Cylinder]
!24 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !25, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!25 = metadata !{null, metadata !17}
!26 = metadata !{i32 786468}
!27 = metadata !{i32 786434, metadata !1, null, metadata !"Circle", i32 2, i64 64, i64 64, i32 0, i32 0, null, metadata !28, i32 0, null, null, metadata !"_ZTS6Circle"} ; [ DW_TAG_class_type ] [Circle] [line 2, size 64, align 64, offset 0] [def] [from ]
!28 = metadata !{metadata !29, metadata !30, metadata !35}
!29 = metadata !{i32 786445, metadata !1, metadata !"_ZTS6Circle", metadata !"radius", i32 3, i64 64, i64 64, i64 0, i32 1, metadata !13} ; [ DW_TAG_member ] [_ZTS6Circle] [radius] [line 3, size 64, align 64, offset 0] [private] [from double]
!30 = metadata !{i32 786478, metadata !1, metadata !"_ZTS6Circle", metadata !"Circle", metadata !"Circle", metadata !"", i32 5, metadata !31, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !34, i32 5} ; [ DW_TAG_subprogram ] [line 5] [Circle]
!31 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !32, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!32 = metadata !{null, metadata !33, metadata !13}
!33 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 1088, metadata !"_ZTS6Circle"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS6Circle]
!34 = metadata !{i32 786468}
!35 = metadata !{i32 786478, metadata !1, metadata !"_ZTS6Circle", metadata !"area", metadata !"area", metadata !"_ZN6Circle4areaEv", i32 6, metadata !36, i1 false, i1 false, i32 0, i32 0, null, i32 256, i1 false, null, null, i32 0, metadata !38, i32 6} ; [ DW_TAG_subprogram ] [line 6] [area]
!36 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !37, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!37 = metadata !{metadata !13, metadata !33}
!38 = metadata !{i32 786468}
!39 = metadata !{metadata !40, metadata !45, metadata !46, metadata !47}
!40 = metadata !{i32 786478, metadata !1, metadata !41, metadata !"main", metadata !"main", metadata !"", i32 26, metadata !42, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 ()* @main, null, null, metadata !2, i32 26} ; [ DW_TAG_subprogram ] [line 26] [def] [main]
!41 = metadata !{i32 786473, metadata !1}         ; [ DW_TAG_file_type ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!42 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !43, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!43 = metadata !{metadata !44}
!44 = metadata !{i32 786468, null, null, metadata !"int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!45 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"~Cylinder", metadata !"~Cylinder", metadata !"_ZN8CylinderD2Ev", i32 21, metadata !24, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Cylinder*)* @_ZN8CylinderD2Ev, null, metadata !23, metadata !2, i32 21} ; [ DW_TAG_subprogram ] [line 21] [def] [~Cylinder]
!46 = metadata !{i32 786478, metadata !1, metadata !"_ZTS8Cylinder", metadata !"Cylinder", metadata !"Cylinder", metadata !"_ZN8CylinderC2Edd", i32 13, metadata !15, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Cylinder*, double, double)* @_ZN8CylinderC2Edd, null, metadata !14, metadata !2, i32 13} ; [ DW_TAG_subprogram ] [line 13] [def] [Cylinder]
!47 = metadata !{i32 786478, metadata !1, metadata !"_ZTS6Circle", metadata !"Circle", metadata !"Circle", metadata !"_ZN6CircleC2Ed", i32 5, metadata !31, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (%class.Circle*, double)* @_ZN6CircleC2Ed, null, metadata !30, metadata !2, i32 5} ; [ DW_TAG_subprogram ] [line 5] [def] [Circle]
!48 = metadata !{metadata !49, metadata !54}
!49 = metadata !{i32 786440, metadata !50, metadata !52, i32 95} ; [ DW_TAG_imported_declaration ]
!50 = metadata !{i32 786489, metadata !51, null, metadata !"std", i32 178} ; [ DW_TAG_namespace ] [std] [line 178]
!51 = metadata !{metadata !"/usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/x86_64-linux-gnu/c++/4.8/bits/c++config.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!52 = metadata !{i32 786454, metadata !53, null, metadata !"FILE", i32 49, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS8_IO_FILE"} ; [ DW_TAG_typedef ] [FILE] [line 49, size 0, align 0, offset 0] [from _ZTS8_IO_FILE]
!53 = metadata !{metadata !"/usr/include/stdio.h", metadata !"/home/anirudh/software/llvm/test/tryLLVM/CPP"}
!54 = metadata !{i32 786440, metadata !50, metadata !55, i32 96} ; [ DW_TAG_imported_declaration ]
!55 = metadata !{i32 786454, metadata !53, null, metadata !"fpos_t", i32 111, i64 0, i64 0, i64 0, i32 0, metadata !56} ; [ DW_TAG_typedef ] [fpos_t] [line 111, size 0, align 0, offset 0] [from _G_fpos_t]
!56 = metadata !{i32 786454, metadata !7, null, metadata !"_G_fpos_t", i32 26, i64 0, i64 0, i64 0, i32 0, metadata !"_ZTS9_G_fpos_t"} ; [ DW_TAG_typedef ] [_G_fpos_t] [line 26, size 0, align 0, offset 0] [from _ZTS9_G_fpos_t]
!57 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!58 = metadata !{i32 1, metadata !"Debug Info Version", i32 1}
!59 = metadata !{metadata !"clang version 3.5.0 "}
!60 = metadata !{i32 786688, metadata !40, metadata !"foo", metadata !41, i32 27, metadata !61, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [foo] [line 27]
!61 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 64, i64 64, i64 0, i32 0, metadata !"_ZTS8Cylinder"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS8Cylinder]
!62 = metadata !{i32 27, i32 0, metadata !40, null}
!63 = metadata !{i32 27, i32 0, metadata !64, null}
!64 = metadata !{i32 786443, metadata !1, metadata !40, i32 27, i32 0, i32 1, i32 3} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!65 = metadata !{i32 28, i32 0, metadata !40, null}
!66 = metadata !{i32 29, i32 0, metadata !40, null}
!67 = metadata !{i32 29, i32 0, metadata !68, null}
!68 = metadata !{i32 786443, metadata !1, metadata !40, i32 29, i32 0, i32 1, i32 4} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!69 = metadata !{i32 29, i32 0, metadata !70, null}
!70 = metadata !{i32 786443, metadata !1, metadata !40, i32 29, i32 0, i32 2, i32 5} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!71 = metadata !{i32 31, i32 0, metadata !40, null}
!72 = metadata !{i32 32, i32 0, metadata !40, null}
!73 = metadata !{i32 32, i32 0, metadata !74, null}
!74 = metadata !{i32 786443, metadata !1, metadata !75, i32 32, i32 0, i32 2, i32 7} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!75 = metadata !{i32 786443, metadata !1, metadata !40, i32 32, i32 0, i32 1, i32 6} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!76 = metadata !{i32 786689, metadata !46, metadata !"this", null, i32 16777216, metadata !61, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!77 = metadata !{i32 0, i32 0, metadata !46, null}
!78 = metadata !{i32 786689, metadata !46, metadata !"r", metadata !41, i32 33554445, metadata !13, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [r] [line 13]
!79 = metadata !{i32 13, i32 0, metadata !46, null}
!80 = metadata !{i32 786689, metadata !46, metadata !"h", metadata !41, i32 50331661, metadata !13, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [h] [line 13]
!81 = metadata !{i32 14, i32 0, metadata !82, null}
!82 = metadata !{i32 786443, metadata !1, metadata !46, i32 13, i32 0, i32 0, i32 1} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!83 = metadata !{i32 14, i32 0, metadata !84, null}
!84 = metadata !{i32 786443, metadata !1, metadata !82, i32 14, i32 0, i32 1, i32 8} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!85 = metadata !{i32 15, i32 0, metadata !82, null}
!86 = metadata !{i32 16, i32 0, metadata !46, null}
!87 = metadata !{i32 16, i32 0, metadata !82, null}
!88 = metadata !{i32 16, i32 0, metadata !89, null}
!89 = metadata !{i32 786443, metadata !1, metadata !82, i32 16, i32 0, i32 1, i32 9} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!90 = metadata !{i32 786689, metadata !45, metadata !"this", null, i32 16777216, metadata !61, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!91 = metadata !{i32 0, i32 0, metadata !45, null}
!92 = metadata !{i32 22, i32 0, metadata !93, null}
!93 = metadata !{i32 786443, metadata !1, metadata !45, i32 21, i32 0, i32 0, i32 0} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!94 = metadata !{i32 22, i32 0, metadata !95, null}
!95 = metadata !{i32 786443, metadata !1, metadata !93, i32 22, i32 0, i32 1, i32 10} ; [ DW_TAG_lexical_block ] [/home/anirudh/software/llvm/test/tryLLVM/CPP/nestedObj.cpp]
!96 = metadata !{i32 23, i32 0, metadata !45, null}
!97 = metadata !{i32 786689, metadata !47, metadata !"this", null, i32 16777216, metadata !11, i32 1088, i32 0} ; [ DW_TAG_arg_variable ] [this] [line 0]
!98 = metadata !{i32 0, i32 0, metadata !47, null}
!99 = metadata !{i32 786689, metadata !47, metadata !"r", metadata !41, i32 33554437, metadata !13, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [r] [line 5]
!100 = metadata !{i32 5, i32 0, metadata !47, null}
