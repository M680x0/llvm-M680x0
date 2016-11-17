; RUN: llc -mtriple=m680x0-linux < %s -verify-machineinstrs | FileCheck %s -check-prefix=x00

; x00-LABEL: test1:
; x00:      eori.l #31
; x00-NEXT: move.l #32
; x00-NEXT: sub.l
; x00-NEXT: rts
define i32 @test1(i32 %x) {
  %xor = xor i32 %x, 31
  %sub = sub i32 32, %xor
  ret i32 %sub
}
