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
; x00:       move.l (%a0), [[HX:%d[0-7]]]
; x00:       add.l  (4,%a0), [[LO:%d[0-7]]]
; x00-NEXT:  addx.l [[HX]], [[HI:%d[0-7]]]
; x00-NEXT:  move.l [[LO]], (4,%a0)
; x00-NEXT:  move.l [[HI]], (%a0)

define fastcc void @test3(i64* inreg %a) nounwind {
  %aa = load i64, i64* %a
  %b = add i64 %aa, 2147483648
  store i64 %b, i64* %a
  ret void
}


; x00-LABEL: test4:
; x00:       move.l (%a0), [[HX:%d[0-7]]]
; x00:       add.l  (4,%a0), [[LO:%d[0-7]]]
; x00-NEXT:  addx.l [[HX]], [[HI:%d[0-7]]]
; x00-NEXT:  move.l [[LO]], (4,%a0)
; x00-NEXT:  move.l [[HI]], (%a0)

define fastcc void @test4(i64* inreg %a) nounwind {
  %aa = load i64, i64* %a
  %b = add i64 %aa, 128
  store i64 %b, i64* %a
  ret void
}


declare {i32, i1} @llvm.sadd.with.overflow.i32(i32, i32)
declare {i32, i1} @llvm.uadd.with.overflow.i32(i32, i32)


; x00-LABEL: test5:
; x00:       add.l
; x00-NEXT:  bvs

define fastcc i32 @test5(i32 %v1, i32 %v2, i32* %X) nounwind {
entry:
  %t = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %v1, i32 %v2)
  %sum = extractvalue {i32, i1} %t, 0
  %obit = extractvalue {i32, i1} %t, 1
  br i1 %obit, label %overflow, label %normal

normal:
  store i32 0, i32* %X
  br label %overflow

overflow:
  ret i32 %sum
}


; x00-LABEL: test6:
; x00:       add.l
; x00-NEXT:  bcs

define fastcc i1 @test6(i32 %v1, i32 %v2, i32* %X) nounwind {
entry:
  %t = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %v1, i32 %v2)
  %sum = extractvalue {i32, i1} %t, 0
  %obit = extractvalue {i32, i1} %t, 1
  br i1 %obit, label %carry, label %normal

normal:
  store i32 0, i32* %X
  br label %carry

carry:
  ret i1 false
}


; x00-LABEL: test7:
; x00:       move.l (4,%sp), %d0
; x00-NEXT:  add.l  (8,%sp), %d0
; x00-NEXT:  scs %d1
; x00-NEXT:  rts

define {i32, i1} @test7(i32 %v1, i32 %v2) nounwind {
  %t = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %v1, i32 %v2)
  ret {i32, i1} %t
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


; x00-LABEL: test10:
; x00:       add.l #1
; x00-NEXT:  svs
; x00-NEXT:  rts

define fastcc i1 @test10(i32 %x) nounwind {
entry:
  %t = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %x, i32 1)
  %obit = extractvalue {i32, i1} %t, 1
  ret i1 %obit
}
