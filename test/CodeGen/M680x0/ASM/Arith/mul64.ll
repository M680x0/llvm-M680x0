; RUN: llc < %s -mtriple=m680x0-linux -verify-machineinstrs | FileCheck %s --check-prefix=x00

; Currenlty making the libcall is ok, x20 supports i32 mul/div which
; yields saner expansion for i64 mul
; x00-LABEL: foo:
; x00:       jsr __muldi3
define i64 @foo(i64 %t, i64 %u) nounwind {
  %k = mul i64 %t, %u
  ret i64 %k
}
