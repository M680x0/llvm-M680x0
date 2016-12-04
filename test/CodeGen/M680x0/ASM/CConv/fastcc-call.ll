; RUN: llc < %s -mtriple=m680x0-pc-linux -relocation-model=pic -verify-machineinstrs | FileCheck -check-prefix=x00 %s 

;
; Pass first 4 arguments in registers %d0,%d1,%a0,%a1 the rest goes onto stack

; x00-LABEL: foo1:
; x00-DAG:   move.l #5, (%sp)
; x00-DAG:   move.l #4, %a1
; x00-DAG:   move.l #3, %a0
; x00-DAG:   move.l #2, %d1
; x00-DAG:   move.l #1, %d0
; x00-NEXT:  jsr (bar1@PLT,%pc)

define i32 @foo1() nounwind uwtable {
entry:
  call fastcc void @bar1(i32 1, i32 2, i32 3, i32 4, i32 5) nounwind
  ret i32 0
}

declare fastcc void @bar1(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e);


;
; Pass pointers in %a registers if there are any free left

; x00-LABEL: foo2:
; x00-DAG:   lea (8,%sp), %a0
; x00-DAG:   lea (4,%sp), %a1
; x00-DAG:   move.l #2, %d0
; x00-DAG:   move.l #4, %d1
; x00-DAG:   jsr (bar2@PLT,%pc)

define i32 @foo2() nounwind uwtable {
entry:
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  call fastcc void @bar2(i32* %a, i32 2, i32* %b, i32 4) nounwind
  ret i32 0
}

declare fastcc void @bar2(i32* %a, i32 %b, i32* %c, i32 %d);
