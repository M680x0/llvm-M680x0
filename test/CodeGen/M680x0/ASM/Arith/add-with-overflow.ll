; RUN: llc < %s -mtriple=m680x0-linux -verify-machineinstrs | FileCheck %s -check-prefix=x00

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
