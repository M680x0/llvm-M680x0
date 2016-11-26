; RUN: llc -mtriple=m680x0-linux -verify-machineinstrs  < %s | FileCheck %s -check-prefix=x00

; x00-LABEL: test1:
; x00:       move.l #-1030792151
; x00:       jsr __mulsi3
define i32 @test1(i32 %x) {
  %div = sdiv exact i32 %x, 25
  ret i32 %div
}

; x00-LABEL: test2:
; x00:       asr.l #3
; x00:       move.l #-1431655765
; x00:       jsr __mulsi3
define i32 @test2(i32 %x) {
  %div = sdiv exact i32 %x, 24
  ret i32 %div
}
