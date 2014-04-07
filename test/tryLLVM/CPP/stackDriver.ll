; ModuleID = 'stackDriver.cpp'
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
%class.Stack = type { i32, i32, i32* }
%class.Stack.0 = type { i32, i32, float* }

@_ZStL8__ioinit = internal global %"class.std::ios_base::Init" zeroinitializer, align 1
@__dso_handle = external global i8
@_ZSt4cout = external global %"class.std::basic_ostream"
@llvm.global_ctors = appending global [1 x { i32, void ()* }] [{ i32, void ()* } { i32 65535, void ()* @_GLOBAL__I_a }]

define internal void @__cxx_global_var_init() section ".text.startup" {
entry:
  call void @_ZNSt8ios_base4InitC1Ev(%"class.std::ios_base::Init"* @_ZStL8__ioinit)
  %0 = call i32 @__cxa_atexit(void (i8*)* bitcast (void (%"class.std::ios_base::Init"*)* @_ZNSt8ios_base4InitD1Ev to void (i8*)*), i8* getelementptr inbounds (%"class.std::ios_base::Init"* @_ZStL8__ioinit, i32 0, i32 0), i8* @__dso_handle) #1
  ret void
}

declare void @_ZNSt8ios_base4InitC1Ev(%"class.std::ios_base::Init"*) #0

declare void @_ZNSt8ios_base4InitD1Ev(%"class.std::ios_base::Init"*) #0

; Function Attrs: nounwind
declare i32 @__cxa_atexit(void (i8*)*, i8*, i8*) #1

; Function Attrs: uwtable
define i32 @main() #2 {
entry:
  %retval = alloca i32, align 4
  %integerStack = alloca %class.Stack, align 8
  %fs = alloca %class.Stack.0, align 8
  %exn.slot = alloca i8*
  %ehselector.slot = alloca i32
  %f = alloca float, align 4
  %i = alloca i32, align 4
  store i32 0, i32* %retval
  call void @_ZN5StackIiEC2Ei(%class.Stack* %integerStack, i32 6)
  invoke void @_ZN5StackIfEC2Ei(%class.Stack.0* %fs, i32 5)
          to label %invoke.cont unwind label %lpad

invoke.cont:                                      ; preds = %entry
  store float 0x3FF19999A0000000, float* %f, align 4
  store i32 1, i32* %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %invoke.cont5, %invoke.cont
  %call = invoke i32 @_ZN5StackIfE4pushERKf(%class.Stack.0* %fs, float* %f)
          to label %invoke.cont2 unwind label %lpad1

invoke.cont2:                                     ; preds = %while.cond
  %tobool = icmp ne i32 %call, 0
  br i1 %tobool, label %while.body, label %while.end

while.body:                                       ; preds = %invoke.cont2
  %0 = load float* %f, align 4
  %call4 = invoke %"class.std::basic_ostream"* @_ZNSolsEf(%"class.std::basic_ostream"* @_ZSt4cout, float %0)
          to label %invoke.cont3 unwind label %lpad1

invoke.cont3:                                     ; preds = %while.body
  %call6 = invoke %"class.std::basic_ostream"* @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_c(%"class.std::basic_ostream"* %call4, i8 signext 32)
          to label %invoke.cont5 unwind label %lpad1

invoke.cont5:                                     ; preds = %invoke.cont3
  %1 = load float* %f, align 4
  %conv = fpext float %1 to double
  %add = fadd double %conv, 1.100000e+00
  %conv7 = fptrunc double %add to float
  store float %conv7, float* %f, align 4
  br label %while.cond

lpad:                                             ; preds = %while.end26, %entry
  %2 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup
  %3 = extractvalue { i8*, i32 } %2, 0
  store i8* %3, i8** %exn.slot
  %4 = extractvalue { i8*, i32 } %2, 1
  store i32 %4, i32* %ehselector.slot
  br label %ehcleanup

lpad1:                                            ; preds = %while.cond21, %while.cond13, %while.cond8, %invoke.cont3, %while.body, %while.cond
  %5 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          cleanup
  %6 = extractvalue { i8*, i32 } %5, 0
  store i8* %6, i8** %exn.slot
  %7 = extractvalue { i8*, i32 } %5, 1
  store i32 %7, i32* %ehselector.slot
  invoke void @_ZN5StackIfED2Ev(%class.Stack.0* %fs)
          to label %invoke.cont28 unwind label %terminate.lpad

while.end:                                        ; preds = %invoke.cont2
  br label %while.cond8

while.cond8:                                      ; preds = %while.end19, %while.end
  %call10 = invoke i32 @_ZN5StackIfE3popERf(%class.Stack.0* %fs, float* %f)
          to label %invoke.cont9 unwind label %lpad1

invoke.cont9:                                     ; preds = %while.cond8
  %tobool11 = icmp ne i32 %call10, 0
  br i1 %tobool11, label %while.body12, label %while.end20

while.body12:                                     ; preds = %invoke.cont9
  br label %while.cond13

while.cond13:                                     ; preds = %while.body17, %while.body12
  %call15 = invoke i32 @_ZN5StackIiE4pushERKi(%class.Stack* %integerStack, i32* %i)
          to label %invoke.cont14 unwind label %lpad1

invoke.cont14:                                    ; preds = %while.cond13
  %tobool16 = icmp ne i32 %call15, 0
  br i1 %tobool16, label %while.body17, label %while.end19

while.body17:                                     ; preds = %invoke.cont14
  %8 = load i32* %i, align 4
  %add18 = add nsw i32 %8, 1
  store i32 %add18, i32* %i, align 4
  br label %while.cond13

while.end19:                                      ; preds = %invoke.cont14
  br label %while.cond8

while.end20:                                      ; preds = %invoke.cont9
  br label %while.cond21

while.cond21:                                     ; preds = %while.body25, %while.end20
  %call23 = invoke i32 @_ZN5StackIiE3popERi(%class.Stack* %integerStack, i32* %i)
          to label %invoke.cont22 unwind label %lpad1

invoke.cont22:                                    ; preds = %while.cond21
  %tobool24 = icmp ne i32 %call23, 0
  br i1 %tobool24, label %while.body25, label %while.end26

while.body25:                                     ; preds = %invoke.cont22
  br label %while.cond21

while.end26:                                      ; preds = %invoke.cont22
  invoke void @_ZN5StackIfED2Ev(%class.Stack.0* %fs)
          to label %invoke.cont27 unwind label %lpad

invoke.cont27:                                    ; preds = %while.end26
  call void @_ZN5StackIiED2Ev(%class.Stack* %integerStack)
  %9 = load i32* %retval
  ret i32 %9

invoke.cont28:                                    ; preds = %lpad1
  br label %ehcleanup

ehcleanup:                                        ; preds = %invoke.cont28, %lpad
  invoke void @_ZN5StackIiED2Ev(%class.Stack* %integerStack)
          to label %invoke.cont29 unwind label %terminate.lpad

invoke.cont29:                                    ; preds = %ehcleanup
  br label %eh.resume

eh.resume:                                        ; preds = %invoke.cont29
  %exn = load i8** %exn.slot
  %sel = load i32* %ehselector.slot
  %lpad.val = insertvalue { i8*, i32 } undef, i8* %exn, 0
  %lpad.val30 = insertvalue { i8*, i32 } %lpad.val, i32 %sel, 1
  resume { i8*, i32 } %lpad.val30

terminate.lpad:                                   ; preds = %ehcleanup, %lpad1
  %10 = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @__gxx_personality_v0 to i8*)
          catch i8* null
  %11 = extractvalue { i8*, i32 } %10, 0
  call void @__clang_call_terminate(i8* %11) #8
  unreachable
}

; Function Attrs: uwtable
define linkonce_odr void @_ZN5StackIiEC2Ei(%class.Stack* %this, i32 %s) unnamed_addr #2 align 2 {
entry:
  %this.addr = alloca %class.Stack*, align 8
  %s.addr = alloca i32, align 4
  store %class.Stack* %this, %class.Stack** %this.addr, align 8
  store i32 %s, i32* %s.addr, align 4
  %this1 = load %class.Stack** %this.addr
  %0 = load i32* %s.addr, align 4
  %cmp = icmp sgt i32 %0, 0
  br i1 %cmp, label %land.lhs.true, label %cond.false

land.lhs.true:                                    ; preds = %entry
  %1 = load i32* %s.addr, align 4
  %cmp2 = icmp slt i32 %1, 1000
  br i1 %cmp2, label %cond.true, label %cond.false

cond.true:                                        ; preds = %land.lhs.true
  %2 = load i32* %s.addr, align 4
  br label %cond.end

cond.false:                                       ; preds = %land.lhs.true, %entry
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi i32 [ %2, %cond.true ], [ 10, %cond.false ]
  %size = getelementptr inbounds %class.Stack* %this1, i32 0, i32 0
  store i32 %cond, i32* %size, align 4
  %top = getelementptr inbounds %class.Stack* %this1, i32 0, i32 1
  store i32 -1, i32* %top, align 4
  %size3 = getelementptr inbounds %class.Stack* %this1, i32 0, i32 0
  %3 = load i32* %size3, align 4
  %4 = sext i32 %3 to i64
  %5 = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %4, i64 4)
  %6 = extractvalue { i64, i1 } %5, 1
  %7 = extractvalue { i64, i1 } %5, 0
  %8 = select i1 %6, i64 -1, i64 %7
  %call = call noalias i8* @_Znam(i64 %8) #9
  %9 = bitcast i8* %call to i32*
  %stackPtr = getelementptr inbounds %class.Stack* %this1, i32 0, i32 2
  store i32* %9, i32** %stackPtr, align 8
  ret void
}

; Function Attrs: uwtable
define linkonce_odr void @_ZN5StackIfEC2Ei(%class.Stack.0* %this, i32 %s) unnamed_addr #2 align 2 {
entry:
  %this.addr = alloca %class.Stack.0*, align 8
  %s.addr = alloca i32, align 4
  store %class.Stack.0* %this, %class.Stack.0** %this.addr, align 8
  store i32 %s, i32* %s.addr, align 4
  %this1 = load %class.Stack.0** %this.addr
  %0 = load i32* %s.addr, align 4
  %cmp = icmp sgt i32 %0, 0
  br i1 %cmp, label %land.lhs.true, label %cond.false

land.lhs.true:                                    ; preds = %entry
  %1 = load i32* %s.addr, align 4
  %cmp2 = icmp slt i32 %1, 1000
  br i1 %cmp2, label %cond.true, label %cond.false

cond.true:                                        ; preds = %land.lhs.true
  %2 = load i32* %s.addr, align 4
  br label %cond.end

cond.false:                                       ; preds = %land.lhs.true, %entry
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi i32 [ %2, %cond.true ], [ 10, %cond.false ]
  %size = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 0
  store i32 %cond, i32* %size, align 4
  %top = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 1
  store i32 -1, i32* %top, align 4
  %size3 = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 0
  %3 = load i32* %size3, align 4
  %4 = sext i32 %3 to i64
  %5 = call { i64, i1 } @llvm.umul.with.overflow.i64(i64 %4, i64 4)
  %6 = extractvalue { i64, i1 } %5, 1
  %7 = extractvalue { i64, i1 } %5, 0
  %8 = select i1 %6, i64 -1, i64 %7
  %call = call noalias i8* @_Znam(i64 %8) #9
  %9 = bitcast i8* %call to float*
  %stackPtr = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 2
  store float* %9, float** %stackPtr, align 8
  ret void
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: uwtable
define linkonce_odr i32 @_ZN5StackIfE4pushERKf(%class.Stack.0* %this, float* %item) #2 align 2 {
entry:
  %retval = alloca i32, align 4
  %this.addr = alloca %class.Stack.0*, align 8
  %item.addr = alloca float*, align 8
  store %class.Stack.0* %this, %class.Stack.0** %this.addr, align 8
  store float* %item, float** %item.addr, align 8
  %this1 = load %class.Stack.0** %this.addr
  %call = call i32 @_ZNK5StackIfE6isFullEv(%class.Stack.0* %this1)
  %tobool = icmp ne i32 %call, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %0 = load float** %item.addr, align 8
  %1 = load float* %0, align 4
  %top = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 1
  %2 = load i32* %top, align 4
  %inc = add nsw i32 %2, 1
  store i32 %inc, i32* %top, align 4
  %idxprom = sext i32 %inc to i64
  %stackPtr = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 2
  %3 = load float** %stackPtr, align 8
  %arrayidx = getelementptr inbounds float* %3, i64 %idxprom
  store float %1, float* %arrayidx, align 4
  store i32 1, i32* %retval
  br label %return

if.end:                                           ; preds = %entry
  store i32 0, i32* %retval
  br label %return

return:                                           ; preds = %if.end, %if.then
  %4 = load i32* %retval
  ret i32 %4
}

declare %"class.std::basic_ostream"* @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_c(%"class.std::basic_ostream"*, i8 signext) #0

declare %"class.std::basic_ostream"* @_ZNSolsEf(%"class.std::basic_ostream"*, float) #0

; Function Attrs: uwtable
define linkonce_odr i32 @_ZN5StackIfE3popERf(%class.Stack.0* %this, float* %popValue) #2 align 2 {
entry:
  %retval = alloca i32, align 4
  %this.addr = alloca %class.Stack.0*, align 8
  %popValue.addr = alloca float*, align 8
  store %class.Stack.0* %this, %class.Stack.0** %this.addr, align 8
  store float* %popValue, float** %popValue.addr, align 8
  %this1 = load %class.Stack.0** %this.addr
  %call = call i32 @_ZNK5StackIfE7isEmptyEv(%class.Stack.0* %this1)
  %tobool = icmp ne i32 %call, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %top = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 1
  %0 = load i32* %top, align 4
  %dec = add nsw i32 %0, -1
  store i32 %dec, i32* %top, align 4
  %idxprom = sext i32 %0 to i64
  %stackPtr = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 2
  %1 = load float** %stackPtr, align 8
  %arrayidx = getelementptr inbounds float* %1, i64 %idxprom
  %2 = load float* %arrayidx, align 4
  %3 = load float** %popValue.addr, align 8
  store float %2, float* %3, align 4
  store i32 1, i32* %retval
  br label %return

if.end:                                           ; preds = %entry
  store i32 0, i32* %retval
  br label %return

return:                                           ; preds = %if.end, %if.then
  %4 = load i32* %retval
  ret i32 %4
}

; Function Attrs: uwtable
define linkonce_odr i32 @_ZN5StackIiE4pushERKi(%class.Stack* %this, i32* %item) #2 align 2 {
entry:
  %retval = alloca i32, align 4
  %this.addr = alloca %class.Stack*, align 8
  %item.addr = alloca i32*, align 8
  store %class.Stack* %this, %class.Stack** %this.addr, align 8
  store i32* %item, i32** %item.addr, align 8
  %this1 = load %class.Stack** %this.addr
  %call = call i32 @_ZNK5StackIiE6isFullEv(%class.Stack* %this1)
  %tobool = icmp ne i32 %call, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %0 = load i32** %item.addr, align 8
  %1 = load i32* %0, align 4
  %top = getelementptr inbounds %class.Stack* %this1, i32 0, i32 1
  %2 = load i32* %top, align 4
  %inc = add nsw i32 %2, 1
  store i32 %inc, i32* %top, align 4
  %idxprom = sext i32 %inc to i64
  %stackPtr = getelementptr inbounds %class.Stack* %this1, i32 0, i32 2
  %3 = load i32** %stackPtr, align 8
  %arrayidx = getelementptr inbounds i32* %3, i64 %idxprom
  store i32 %1, i32* %arrayidx, align 4
  store i32 1, i32* %retval
  br label %return

if.end:                                           ; preds = %entry
  store i32 0, i32* %retval
  br label %return

return:                                           ; preds = %if.end, %if.then
  %4 = load i32* %retval
  ret i32 %4
}

; Function Attrs: uwtable
define linkonce_odr i32 @_ZN5StackIiE3popERi(%class.Stack* %this, i32* %popValue) #2 align 2 {
entry:
  %retval = alloca i32, align 4
  %this.addr = alloca %class.Stack*, align 8
  %popValue.addr = alloca i32*, align 8
  store %class.Stack* %this, %class.Stack** %this.addr, align 8
  store i32* %popValue, i32** %popValue.addr, align 8
  %this1 = load %class.Stack** %this.addr
  %call = call i32 @_ZNK5StackIiE7isEmptyEv(%class.Stack* %this1)
  %tobool = icmp ne i32 %call, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %top = getelementptr inbounds %class.Stack* %this1, i32 0, i32 1
  %0 = load i32* %top, align 4
  %dec = add nsw i32 %0, -1
  store i32 %dec, i32* %top, align 4
  %idxprom = sext i32 %0 to i64
  %stackPtr = getelementptr inbounds %class.Stack* %this1, i32 0, i32 2
  %1 = load i32** %stackPtr, align 8
  %arrayidx = getelementptr inbounds i32* %1, i64 %idxprom
  %2 = load i32* %arrayidx, align 4
  %3 = load i32** %popValue.addr, align 8
  store i32 %2, i32* %3, align 4
  store i32 1, i32* %retval
  br label %return

if.end:                                           ; preds = %entry
  store i32 0, i32* %retval
  br label %return

return:                                           ; preds = %if.end, %if.then
  %4 = load i32* %retval
  ret i32 %4
}

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN5StackIfED2Ev(%class.Stack.0* %this) unnamed_addr #3 align 2 {
entry:
  %this.addr = alloca %class.Stack.0*, align 8
  store %class.Stack.0* %this, %class.Stack.0** %this.addr, align 8
  %this1 = load %class.Stack.0** %this.addr
  %stackPtr = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 2
  %0 = load float** %stackPtr, align 8
  %isnull = icmp eq float* %0, null
  br i1 %isnull, label %delete.end, label %delete.notnull

delete.notnull:                                   ; preds = %entry
  %1 = bitcast float* %0 to i8*
  call void @_ZdaPv(i8* %1) #10
  br label %delete.end

delete.end:                                       ; preds = %delete.notnull, %entry
  ret void
}

; Function Attrs: noinline noreturn nounwind
define linkonce_odr hidden void @__clang_call_terminate(i8*) #4 {
  %2 = call i8* @__cxa_begin_catch(i8* %0) #1
  call void @_ZSt9terminatev() #8
  unreachable
}

declare i8* @__cxa_begin_catch(i8*)

declare void @_ZSt9terminatev()

; Function Attrs: nounwind uwtable
define linkonce_odr void @_ZN5StackIiED2Ev(%class.Stack* %this) unnamed_addr #3 align 2 {
entry:
  %this.addr = alloca %class.Stack*, align 8
  store %class.Stack* %this, %class.Stack** %this.addr, align 8
  %this1 = load %class.Stack** %this.addr
  %stackPtr = getelementptr inbounds %class.Stack* %this1, i32 0, i32 2
  %0 = load i32** %stackPtr, align 8
  %isnull = icmp eq i32* %0, null
  br i1 %isnull, label %delete.end, label %delete.notnull

delete.notnull:                                   ; preds = %entry
  %1 = bitcast i32* %0 to i8*
  call void @_ZdaPv(i8* %1) #10
  br label %delete.end

delete.end:                                       ; preds = %delete.notnull, %entry
  ret void
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZNK5StackIiE7isEmptyEv(%class.Stack* %this) #3 align 2 {
entry:
  %this.addr = alloca %class.Stack*, align 8
  store %class.Stack* %this, %class.Stack** %this.addr, align 8
  %this1 = load %class.Stack** %this.addr
  %top = getelementptr inbounds %class.Stack* %this1, i32 0, i32 1
  %0 = load i32* %top, align 4
  %cmp = icmp eq i32 %0, -1
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZNK5StackIiE6isFullEv(%class.Stack* %this) #3 align 2 {
entry:
  %this.addr = alloca %class.Stack*, align 8
  store %class.Stack* %this, %class.Stack** %this.addr, align 8
  %this1 = load %class.Stack** %this.addr
  %top = getelementptr inbounds %class.Stack* %this1, i32 0, i32 1
  %0 = load i32* %top, align 4
  %size = getelementptr inbounds %class.Stack* %this1, i32 0, i32 0
  %1 = load i32* %size, align 4
  %sub = sub nsw i32 %1, 1
  %cmp = icmp eq i32 %0, %sub
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZNK5StackIfE7isEmptyEv(%class.Stack.0* %this) #3 align 2 {
entry:
  %this.addr = alloca %class.Stack.0*, align 8
  store %class.Stack.0* %this, %class.Stack.0** %this.addr, align 8
  %this1 = load %class.Stack.0** %this.addr
  %top = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 1
  %0 = load i32* %top, align 4
  %cmp = icmp eq i32 %0, -1
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: nounwind uwtable
define linkonce_odr i32 @_ZNK5StackIfE6isFullEv(%class.Stack.0* %this) #3 align 2 {
entry:
  %this.addr = alloca %class.Stack.0*, align 8
  store %class.Stack.0* %this, %class.Stack.0** %this.addr, align 8
  %this1 = load %class.Stack.0** %this.addr
  %top = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 1
  %0 = load i32* %top, align 4
  %size = getelementptr inbounds %class.Stack.0* %this1, i32 0, i32 0
  %1 = load i32* %size, align 4
  %sub = sub nsw i32 %1, 1
  %cmp = icmp eq i32 %0, %sub
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; Function Attrs: nobuiltin nounwind
declare void @_ZdaPv(i8*) #5

; Function Attrs: nounwind readnone
declare { i64, i1 } @llvm.umul.with.overflow.i64(i64, i64) #6

; Function Attrs: nobuiltin
declare noalias i8* @_Znam(i64) #7

define internal void @_GLOBAL__I_a() section ".text.startup" {
entry:
  call void @__cxx_global_var_init()
  ret void
}

attributes #0 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }
attributes #2 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { noinline noreturn nounwind }
attributes #5 = { nobuiltin nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { nounwind readnone }
attributes #7 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #8 = { noreturn nounwind }
attributes #9 = { builtin }
attributes #10 = { builtin nounwind }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.5.0 "}
