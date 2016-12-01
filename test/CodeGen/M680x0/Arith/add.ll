; RUN: llc < %s -mtriple=m680x0-linux -verify-machineinstrs | FileCheck %s -check-prefix=x00

; x00-LABEL: test1:
; x00:      move.l (12,%sp), %d0
; x00-NEXT: add.l  (4,%sp),  %d0
; x00-NEXT: move.l (8,%sp),  %d1
; x00-NEXT: rts

define i64 @test1(i64 %A, i32 %B) nounwind {
  %tmp12 = zext i32 %B to i64
  %tmp3 = shl i64 %tmp12, 32
  %tmp5 = add i64 %tmp3, %A
  ret i64 %tmp5
}


; x00-LABEL: test2:
; x00:       move.l %d0, [[Addr:%a[0-7]]]
; x00-NEXT:  add.l  #128, ([[Addr]])

define void @test2(i32* inreg %a) nounwind {
  %aa = load i32, i32* %a
  %b = add i32 %aa, 128
  store i32 %b, i32* %a
  ret void
}


; x00-LABEL: test2_fast:
; x00:       add.l  #128, (%a0)

define fastcc void @test2_fast(i32* inreg %a) nounwind {
  %aa = load i32, i32* %a
  %b = add i32 %aa, 128
  store i32 %b, i32* %a
  ret void
}


; x00-LABEL: test3:
; x00        move.l (%a0), %d0
; x00-NEXT   move.l #-2147483648, %d1
; x00-NEXT   add.l (4,%a0), %d1
; x00-NEXT   addx.l #0, %d0
; x00-NEXT   move.l %d1, (4,%a0)
; x00-NEXT   move.l %d0, (%a0)
; x00-NEXT   rts
define fastcc void @test3(i64* inreg %a) nounwind {
  %aa = load i64, i64* %a
  %b = add i64 %aa, 2147483648
  store i64 %b, i64* %a
  ret void
}


; x00-LABEL: test4:
; x00 move.l #128, %d0
; x00-NEXT   add.l  (4,%a0), %d0
; x00-NEXT   move.l (%a0), %d1
; x00-NEXT   move.l %d0, (4,%a0)
; x00-NEXT   addx.l #0, %d1
; x00-NEXT   move.l %d1, (%a0)
; x00-NEXT   rts
define fastcc void @test4(i64* inreg %a) nounwind {
  %aa = load i64, i64* %a
  %b = add i64 %aa, 128
  store i64 %b, i64* %a
  ret void
}

; x00-LABEL: test9:
; x00:       sub.l #10
; x00-NEXT:  seq
; x00-NEXT:  and.l
; x00-NEXT:  sub.l
; x00-NEXT:  move.l
; x00-NEXT:  rts

define fastcc i32 @test9(i32 %x, i32 %y) nounwind readnone {
  %cmp = icmp eq i32 %x, 10
  %sub = sext i1 %cmp to i32
  %cond = add i32 %sub, %y
  ret i32 %cond
}
