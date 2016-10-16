; RUN: llc < %s -mtriple=m680x0-pc-linux -relocation-model=pic -verify-machineinstrs | FileCheck -check-prefix=x00 %s 

;
; Pass all arguments on the stack in reverse order

; x00-LABEL: foo:
; x00-DAG:  move.l #5, (#16,%sp)
; x00-DAG:  move.l #4, (#12,%sp)
; x00-DAG:  move.l #3, (#8,%sp)
; x00-DAG:  move.l #2, (#4,%sp)
; x00-DAG:  move.l #1, (%sp)
; x00-NEXT:  jsr (bar@PLT,%pc)

define i32 @foo() nounwind uwtable {
entry:
  call void @bar(i32 1, i32 2, i32 3, i32 4, i32 5) nounwind
  ret i32 0
}

declare void @bar(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e);

