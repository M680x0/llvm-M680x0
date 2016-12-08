; RUN: llc < %s -mtriple=m680x0-linux -verify-machineinstrs | FileCheck %s -check-prefix=x00

; TODO fold the shifts
; x00-LABEL: test1:
; x00:       mulu #-1985
; x00-NEXT:  lsr.l #8
; x00-NEXT:  lsr.l #8
; x00-NEXT:  lsr.w #5
define zeroext i16 @test1(i16 zeroext %x) nounwind {
entry:
	%div = udiv i16 %x, 33
	ret i16 %div
}

; x00-LABEL: test2:
; x00:       mulu #-21845
; x00-NEXT:  lsr.l #8
; x00-NEXT:  lsr.l #8
; x00-NEXT:  lsr.w #1
define zeroext i16 @test2(i8 signext %x, i16 zeroext %c) {
entry:
  %div = udiv i16 %c, 3
  ret i16 %div
}


; x00-LABEL: test3:
; x00:       move.b (11
; x00-NEXT:  and.l #255
; x00-NEXT:  muls #171
; x00-NEXT:  move.w #9
; x00-NEXT:  lsr.w
define zeroext i8 @test3(i8 zeroext %x, i8 zeroext %c) {
entry:
  %div = udiv i8 %c, 3
  ret i8 %div
}

; x00-LABEL: test4:
; x00:       muls #1986
define signext i16 @test4(i16 signext %x) nounwind {
entry:
	%div = sdiv i16 %x, 33		; <i32> [#uses=1]
	ret i16 %div
}

; x00-LABEL: test5:
; x00:       jsr __udivsi3
define i32 @test5(i32 %A) nounwind {
  %tmp1 = udiv i32 %A, 1577682821         ; <i32> [#uses=1]
  ret i32 %tmp1
}

; TODO fold shift
; x00-LABEL: test6:
; x00:       muls #26215
; x00:       move.w #15
; x00:       lsr.w
; x00:       asr.w #2
define signext i16 @test6(i16 signext %x) nounwind {
entry:
  %div = sdiv i16 %x, 10
  ret i16 %div
}

; x00-LABEL: test7:
; x00:       jsr __udivsi3
define i32 @test7(i32 %x) nounwind {
  %div = udiv i32 %x, 28
  ret i32 %div
}

; x00-LABEL: test8:
; x00:       lsr.b #1
; x00:       and.l #255
; x00:       muls #211
; x00:       move.w #13
; x00:       lsr.w
define i8 @test8(i8 %x) nounwind {
  %div = udiv i8 %x, 78
  ret i8 %div
}

; x00-LABEL: test9:
; x00:       lsr.b #2
; x00:       and.l #255
; x00:       muls #71
; x00:       move.w #11
; x00:       lsr.w
define i8 @test9(i8 %x) nounwind {
  %div = udiv i8 %x, 116
  ret i8 %div
}

; x00-LABEL: testsize1:
; x00:       asr.l
; x00:       lsr.l
; x00:       asr.l
define i32 @testsize1(i32 %x) minsize nounwind {
entry:
	%div = sdiv i32 %x, 32
	ret i32 %div
}

; x00-LABEL: testsize2:
; x00:       jsr __divsi3
define i32 @testsize2(i32 %x) minsize nounwind {
entry:
	%div = sdiv i32 %x, 33
	ret i32 %div
}

; x00-LABEL: testsize3:
; x00:       lsr.l #5
define i32 @testsize3(i32 %x) minsize nounwind {
entry:
	%div = udiv i32 %x, 32
	ret i32 %div
}

; x00-LABEL: testsize4:
; x00:       jsr __udivsi3
define i32 @testsize4(i32 %x) minsize nounwind {
entry:
	%div = udiv i32 %x, 33
	ret i32 %div
}
