; RUN: llc < %s -mtriple=m680x0-linux -verify-machineinstrs | FileCheck %s -check-prefix=x00

; x00:     t1
; x00:     lsr.l
; x00-NOT: lsr.l
; x00:     rts

define i1 @t1(i64 %x) nounwind {
  %B = icmp slt i64 %x, 0
  ret i1 %B
}


; x00:      t2
; x00:      cmpi.l
; x00-NEXT: seq
; x00-NEXT: rts

define i1 @t2(i64 %x) nounwind {
  %tmp = icmp ult i64 %x, 4294967296
  ret i1 %tmp
}


; x00: t3
; x00: move.b #0, %d0
; x00: rts

define i1 @t3(i32 %x) nounwind {
  %tmp = icmp ugt i32 %x, -1
  ret i1 %tmp
}
