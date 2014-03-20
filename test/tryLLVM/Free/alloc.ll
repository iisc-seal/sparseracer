; ModuleID = 'alloc.cpp'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%"class.std::ios_base::Init" = type { i8 }
%"class.std::basic_ostream" = type { i32 (...)**, %"class.std::basic_ios" }
%"class.std::basic_ios" = type { %"class.std::ios_base", %"class.std::basic_ostream"*, i8, i8, %"class.std::basic_streambuf"*, %"class.std::ctype"*, %"class.std::num_put"*, %"class.std::num_get"* }
%"class.std::ios_base" = type { i32 (...)**, i64, i64, i32, i32, i32, %"struct.std::ios_base::_Callback_list"*, %"struct.std::ios_base::_Words", [8 x %"struct.std::ios_base::_Words"], i32, %"struct.std::ios_base::_Words"*, %"class.std::locale" }
%"struct.std::ios_base::_Callback_list" = type { %"struct.std::ios_base::_Callback_list"*, void (i32, %"class.std::ios_base"*, i32)*, i32, i32 }
%"struct.std::ios_base::_Words" = type { i8*, i64 }
%"class.std::locale" = type { %"class.std::locale::_Impl"* }
%"class.std::locale::_Impl" = type { i32, %"class.std::locale::facet"**, i64, %"class.std::locale::facet"**, i8** }
%"class.std::locale::facet" = type { i32 (...)**, i32 }
%"class.std::basic_streambuf" = type { i32 (...)**, i8*, i8*, i8*, i8*, i8*, i8*, %"class.std::locale" }
%"class.std::ctype" = type { %"class.std::locale::facet.base", %struct.__locale_struct*, i8, i32*, i32*, i16*, i8, [256 x i8], [256 x i8], i8 }
%"class.std::locale::facet.base" = type <{ i32 (...)**, i32 }>
%struct.__locale_struct = type { [13 x %struct.__locale_data*], i16*, i32*, i32*, [13 x i8*] }
%struct.__locale_data = type opaque
%"class.std::num_put" = type { %"class.std::locale::facet.base", [4 x i8] }
%"class.std::num_get" = type { %"class.std::locale::facet.base", [4 x i8] }
%class.CCC = type { i32, double }

@_ZStL8__ioinit = internal global %"class.std::ios_base::Init" zeroinitializer, align 1
@__dso_handle = external global i8
@_ZSt4cout = external global %"class.std::basic_ostream"
@llvm.global_ctors = appending global [1 x { i32, void ()* }] [{ i32, void ()* } { i32 65535, void ()* @_GLOBAL__I_a }]

@_ZN3CCCC1Ei = alias void (%class.CCC*, i32)* @_ZN3CCCC2Ei
@_ZN3CCCC1Eid = alias void (%class.CCC*, i32, double)* @_ZN3CCCC2Eid

define internal void @__cxx_global_var_init() section ".text.startup" {
  call void @_ZNSt8ios_base4InitC1Ev(%"class.std::ios_base::Init"* @_ZStL8__ioinit)
  %1 = call i32 @__cxa_atexit(void (i8*)* bitcast (void (%"class.std::ios_base::Init"*)* @_ZNSt8ios_base4InitD1Ev to void (i8*)*), i8* getelementptr inbounds (%"class.std::ios_base::Init"* @_ZStL8__ioinit, i32 0, i32 0), i8* @__dso_handle) #1
  ret void
}

declare void @_ZNSt8ios_base4InitC1Ev(%"class.std::ios_base::Init"*) #0

declare void @_ZNSt8ios_base4InitD1Ev(%"class.std::ios_base::Init"*) #0

; Function Attrs: nounwind
declare i32 @__cxa_atexit(void (i8*)*, i8*, i8*) #1

; Function Attrs: nounwind uwtable
define void @_ZN3CCCC2Ei(%class.CCC* %this, i32 %_ii) unnamed_addr #2 align 2 {
  %1 = alloca %class.CCC*, align 8
  %2 = alloca i32, align 4
  store %class.CCC* %this, %class.CCC** %1, align 8
  store i32 %_ii, i32* %2, align 4
  %3 = load %class.CCC** %1
  %4 = getelementptr inbounds %class.CCC* %3, i32 0, i32 0
  %5 = load i32* %2, align 4
  store i32 %5, i32* %4, align 4
  ret void
}

; Function Attrs: nounwind uwtable
define void @_ZN3CCCC2Eid(%class.CCC* %this, i32 %_ii, double %_dd) unnamed_addr #2 align 2 {
  %1 = alloca %class.CCC*, align 8
  %2 = alloca i32, align 4
  %3 = alloca double, align 8
  store %class.CCC* %this, %class.CCC** %1, align 8
  store i32 %_ii, i32* %2, align 4
  store double %_dd, double* %3, align 8
  %4 = load %class.CCC** %1
  %5 = getelementptr inbounds %class.CCC* %4, i32 0, i32 0
  %6 = load i32* %2, align 4
  store i32 %6, i32* %5, align 4
  %7 = getelementptr inbounds %class.CCC* %4, i32 0, i32 1
  %8 = load double* %3, align 8
  store double %8, double* %7, align 8
  ret void
}

; Function Attrs: uwtable
define i32 @main() #3 {
  %1 = alloca i32, align 4
  %cc1 = alloca %class.CCC*, align 8
  %2 = alloca i8*
  %3 = alloca i32
  %cc2 = alloca %class.CCC*, align 8
  %cc3 = alloca %class.CCC*, align 8
  %c4 = alloca %class.CCC**, align 8
  store i32 0, i32* %1
  %4 = call noalias i8* @_Znwm(i64 16) #6
  %5 = bitcast i8* %4 to %class.CCC*
  invoke void @_ZN3CCCC1Eid(%class.CCC* %5, i32 4, double 5.500000e+00)
          to label %6 unwind label %103

; <label>:6                                       ; preds = %0
  store %class.CCC* %5, %class.CCC** %cc1, align 8
  %7 = call noalias i8* @_Znam(i64 80) #6
  %8 = bitcast i8* %7 to %class.CCC*
  %9 = getelementptr inbounds %class.CCC* %8, i64 5
  br label %10

; <label>:10                                      ; preds = %12, %6
  %11 = phi %class.CCC* [ %8, %6 ], [ %13, %12 ]
  invoke void @_ZN3CCCC2Ev(%class.CCC* %11)
          to label %12 unwind label %107

; <label>:12                                      ; preds = %10
  %13 = getelementptr inbounds %class.CCC* %11, i64 1
  %14 = icmp eq %class.CCC* %13, %9
  br i1 %14, label %15, label %10

; <label>:15                                      ; preds = %12
  store %class.CCC* %8, %class.CCC** %cc2, align 8
  %16 = call noalias i8* @_Znwm(i64 16) #6
  %17 = bitcast i8* %16 to %class.CCC*
  invoke void @_ZN3CCCC2Ev(%class.CCC* %17)
          to label %18 unwind label %111

; <label>:18                                      ; preds = %15
  store %class.CCC* %17, %class.CCC** %cc3, align 8
  %19 = call noalias i8* @_Znam(i64 40) #6
  %20 = bitcast i8* %19 to %class.CCC**
  store %class.CCC** %20, %class.CCC*** %c4, align 8
  %21 = load %class.CCC** %cc1, align 8
  %22 = getelementptr inbounds %class.CCC* %21, i32 0, i32 0
  store i32 5, i32* %22, align 4
  %23 = load %class.CCC** %cc2, align 8
  %24 = getelementptr inbounds %class.CCC* %23, i64 3
  %25 = getelementptr inbounds %class.CCC* %24, i32 0, i32 0
  store i32 6, i32* %25, align 4
  %26 = load %class.CCC** %cc3, align 8
  %27 = getelementptr inbounds %class.CCC* %26, i32 0, i32 0
  store i32 7, i32* %27, align 4
  %28 = call noalias i8* @_Znwm(i64 16) #6
  %29 = bitcast i8* %28 to %class.CCC*
  invoke void @_ZN3CCCC1Ei(%class.CCC* %29, i32 8)
          to label %30 unwind label %115

; <label>:30                                      ; preds = %18
  %31 = load %class.CCC*** %c4, align 8
  %32 = getelementptr inbounds %class.CCC** %31, i64 0
  store %class.CCC* %29, %class.CCC** %32, align 8
  %33 = call noalias i8* @_Znwm(i64 16) #6
  %34 = bitcast i8* %33 to %class.CCC*
  invoke void @_ZN3CCCC1Ei(%class.CCC* %34, i32 9)
          to label %35 unwind label %119

; <label>:35                                      ; preds = %30
  %36 = load %class.CCC*** %c4, align 8
  %37 = getelementptr inbounds %class.CCC** %36, i64 1
  store %class.CCC* %34, %class.CCC** %37, align 8
  %38 = load %class.CCC** %cc1, align 8
  %39 = getelementptr inbounds %class.CCC* %38, i32 0, i32 0
  %40 = load i32* %39, align 4
  %41 = call %"class.std::basic_ostream"* @_ZNSolsEi(%"class.std::basic_ostream"* @_ZSt4cout, i32 %40)
  %42 = call %"class.std::basic_ostream"* @_ZNSolsEPFRSoS_E(%"class.std::basic_ostream"* %41, %"class.std::basic_ostream"* (%"class.std::basic_ostream"*)* @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_)
  %43 = load %class.CCC** %cc2, align 8
  %44 = getelementptr inbounds %class.CCC* %43, i64 3
  %45 = getelementptr inbounds %class.CCC* %44, i32 0, i32 0
  %46 = load i32* %45, align 4
  %47 = call %"class.std::basic_ostream"* @_ZNSolsEi(%"class.std::basic_ostream"* @_ZSt4cout, i32 %46)
  %48 = call %"class.std::basic_ostream"* @_ZNSolsEPFRSoS_E(%"class.std::basic_ostream"* %47, %"class.std::basic_ostream"* (%"class.std::basic_ostream"*)* @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_)
  %49 = load %class.CCC** %cc3, align 8
  %50 = getelementptr inbounds %class.CCC* %49, i32 0, i32 0
  %51 = load i32* %50, align 4
  %52 = call %"class.std::basic_ostream"* @_ZNSolsEi(%"class.std::basic_ostream"* @_ZSt4cout, i32 %51)
  %53 = call %"class.std::basic_ostream"* @_ZNSolsEPFRSoS_E(%"class.std::basic_ostream"* %52, %"class.std::basic_ostream"* (%"class.std::basic_ostream"*)* @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_)
  %54 = load %class.CCC*** %c4, align 8
  %55 = getelementptr inbounds %class.CCC** %54, i64 0
  %56 = load %class.CCC** %55, align 8
  %57 = getelementptr inbounds %class.CCC* %56, i32 0, i32 0
  %58 = load i32* %57, align 4
  %59 = call %"class.std::basic_ostream"* @_ZNSolsEi(%"class.std::basic_ostream"* @_ZSt4cout, i32 %58)
  %60 = call %"class.std::basic_ostream"* @_ZNSolsEPFRSoS_E(%"class.std::basic_ostream"* %59, %"class.std::basic_ostream"* (%"class.std::basic_ostream"*)* @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_)
  %61 = load %class.CCC*** %c4, align 8
  %62 = getelementptr inbounds %class.CCC** %61, i64 1
  %63 = load %class.CCC** %62, align 8
  %64 = getelementptr inbounds %class.CCC* %63, i32 0, i32 0
  %65 = load i32* %64, align 4
  %66 = call %"class.std::basic_ostream"* @_ZNSolsEi(%"class.std::basic_ostream"* @_ZSt4cout, i32 %65)
  %67 = call %"class.std::basic_ostream"* @_ZNSolsEPFRSoS_E(%"class.std::basic_ostream"* %66, %"class.std::basic_ostream"* (%"class.std::basic_ostream"*)* @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_)
  %68 = load %class.CCC** %cc1, align 8
  %69 = icmp eq %class.CCC* %68, null
  br i1 %69, label %72, label %70

; <label>:70                                      ; preds = %35
  %71 = bitcast %class.CCC* %68 to i8*
  call void @_ZdlPv(i8* %71) #7
  br label %72

; <label>:72                                      ; preds = %70, %35
  %73 = load %class.CCC** %cc2, align 8
  %74 = icmp eq %class.CCC* %73, null
  br i1 %74, label %77, label %75

; <label>:75                                      ; preds = %72
  %76 = bitcast %class.CCC* %73 to i8*
  call void @_ZdaPv(i8* %76) #7
  br label %77

; <label>:77                                      ; preds = %75, %72
  %78 = load %class.CCC** %cc3, align 8
  %79 = icmp eq %class.CCC* %78, null
  br i1 %79, label %82, label %80

; <label>:80                                      ; preds = %77
  %81 = bitcast %class.CCC* %78 to i8*
  call void @_ZdlPv(i8* %81) #7
  br label %82

; <label>:82                                      ; preds = %80, %77
  %83 = load %class.CCC*** %c4, align 8
  %84 = getelementptr inbounds %class.CCC** %83, i64 0
  %85 = load %class.CCC** %84, align 8
  %86 = icmp eq %class.CCC* %85, null
  br i1 %86, label %89, label %87

; <label>:87                                      ; preds = %82
  %88 = bitcast %class.CCC* %85 to i8*
  call void @_ZdaPv(i8* %88) #7
  br label %89

; <label>:89                                      ; preds = %87, %82
  %90 = load %class.CCC*** %c4, align 8
  %91 = getelementptr inbounds %class.CCC** %90, i64 1
  %92 = load %class.CCC** %91, align 8
  %93 = icmp eq %class.CCC* %92, null
  br i1 %93, label %96, label %94

; <label>:94                                      ; preds = %89
  %95 = bitcast %class.CCC* %92 to i8*
  call void @_ZdaPv(i8* %95) #7
  br label %96

; <label>:96                                      ; preds = %94, %89
  %97 = load %class.CCC*** %c4, align 8
  %98 = icmp eq %class.CCC** %97, null
  br i1 %98, label %101, label %99

; <label>:99                                      ; preds = %96
  %100 = bitcast %class.CCC** %97 to i8*
  call void @_ZdaPv(i8* %100) #7
  br label %101

; <label>:101                                     ; preds = %99, %96
  %102 = load i32* %1
  ret i32 %102

; <label>:103                                     ; preds = %0
  %104 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup
  %105 = extractvalue { i8*, i32 } %104, 0
  store i8* %105, i8** %2
  %106 = extractvalue { i8*, i32 } %104, 1
  store i32 %106, i32* %3
  call void @_ZdlPv(i8* %4) #7
  br label %123

; <label>:107                                     ; preds = %10
  %108 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup
  %109 = extractvalue { i8*, i32 } %108, 0
  store i8* %109, i8** %2
  %110 = extractvalue { i8*, i32 } %108, 1
  store i32 %110, i32* %3
  call void @_ZdaPv(i8* %7) #7
  br label %123

; <label>:111                                     ; preds = %15
  %112 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup
  %113 = extractvalue { i8*, i32 } %112, 0
  store i8* %113, i8** %2
  %114 = extractvalue { i8*, i32 } %112, 1
  store i32 %114, i32* %3
  call void @_ZdlPv(i8* %16) #7
  br label %123

; <label>:115                                     ; preds = %18
  %116 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup
  %117 = extractvalue { i8*, i32 } %116, 0
  store i8* %117, i8** %2
  %118 = extractvalue { i8*, i32 } %116, 1
  store i32 %118, i32* %3
  call void @_ZdlPv(i8* %28) #7
  br label %123

; <label>:119                                     ; preds = %30
  %120 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup
  %121 = extractvalue { i8*, i32 } %120, 0
  store i8* %121, i8** %2
  %122 = extractvalue { i8*, i32 } %120, 1
  store i32 %122, i32* %3
  call void @_ZdlPv(i8* %33) #7
  br label %123

; <label>:123                                     ; preds = %119, %115, %111, %107, %103
  %124 = load i8** %2
  %125 = load i32* %3
  %126 = insertvalue { i8*, i32 } undef, i8* %124, 0
  %127 = insertvalue { i8*, i32 } %126, i32 %125, 1
  resume { i8*, i32 } %127
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #4

declare i32 @__gxx_personality_v0(...)

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(i8*) #5

; Function Attrs: nobuiltin
declare noalias i8* @_Znam(i64) #4

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN3CCCC2Ev(%class.CCC* %this) unnamed_addr #2 align 2 {
  %1 = alloca %class.CCC*, align 8
  store %class.CCC* %this, %class.CCC** %1, align 8
  %2 = load %class.CCC** %1
  ret void
}

; Function Attrs: nobuiltin nounwind
declare void @_ZdaPv(i8*) #5

declare %"class.std::basic_ostream"* @_ZNSolsEi(%"class.std::basic_ostream"*, i32) #0

declare %"class.std::basic_ostream"* @_ZNSolsEPFRSoS_E(%"class.std::basic_ostream"*, %"class.std::basic_ostream"* (%"class.std::basic_ostream"*)*) #0

declare %"class.std::basic_ostream"* @_ZSt4endlIcSt11char_traitsIcEERSt13basic_ostreamIT_T0_ES6_(%"class.std::basic_ostream"*) #0

define internal void @_GLOBAL__I_a() section ".text.startup" {
  call void @__cxx_global_var_init()
  ret void
}

attributes #0 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }
attributes #2 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { nobuiltin nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { builtin }
attributes #7 = { builtin nounwind }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.5.0 "}
