; RUN: llc < %s -mtriple=m680x0-pc-linux -relocation-model=pic -verify-machineinstrs | FileCheck -check-prefix=x00 %s 

; Use jsr and PLT reference
;
; x00-LABEL: far:
; x00:         jsr (foo@PLT,%pc)
; x00:         rts

define i32 @far() nounwind uwtable {
entry:
  call void @foo() nounwind
  ret i32 0
}

declare void @foo()
