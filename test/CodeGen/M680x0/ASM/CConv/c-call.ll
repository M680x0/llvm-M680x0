; RUN: llc < %s -mtriple=m680x0-pc-linux -relocation-model=pic -verify-machineinstrs | FileCheck -check-prefix=x00 %s 

;
; Pass all arguments on the stack in reverse order

; x00-LABEL: test1:
; x00-DAG:  move.l #5, (16,%sp)
; x00-DAG:  move.l #4, (12,%sp)
; x00-DAG:  move.l #3, (8,%sp)
; x00-DAG:  move.l #2, (4,%sp)
; x00-DAG:  move.l #1, (%sp)
; x00-NEXT: jsr (test1_callee@PLT,%pc)
; x00-NEXT: move.l

define i32 @test1() nounwind uwtable {
entry:
  call void @test1_callee(i32 1, i32 2, i32 3, i32 4, i32 5) nounwind
  ret i32 0
}

declare void @test1_callee(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e);

; x00-LABEL: test2:
; x00-DAG:  move.l #5, (16,%sp)
; x00-DAG:  move.l #4, (12,%sp)
; x00-DAG:  move.l #3, (8,%sp)
; x00-DAG:  move.l #2, (4,%sp)
; x00-DAG:  move.l #1, (%sp)
; x00-NEXT: jsr (test2_callee@PLT,%pc)
; x00-NEXT: move.w

define i16 @test2() nounwind uwtable {
entry:
  call void @test2_callee(i16 1, i16 2, i16 3, i16 4, i16 5) nounwind
  ret i16 0
}

declare void @test2_callee(i16 %a, i16 %b, i16 %c, i16 %d, i16 %e);

; x00-LABEL: test3:
; x00-DAG:  move.l #5, (16,%sp)
; x00-DAG:  move.l #4, (12,%sp)
; x00-DAG:  move.l #3, (8,%sp)
; x00-DAG:  move.l #2, (4,%sp)
; x00-DAG:  move.l #1, (%sp)
; x00-NEXT: jsr (test3_callee@PLT,%pc)
; x00-NEXT: move.b

define i8 @test3() nounwind uwtable {
entry:
  call void @test3_callee(i8 1, i8 2, i8 3, i8 4, i8 5) nounwind
  ret i8 0
}

declare void @test3_callee(i8 %a, i8 %b, i8 %c, i8 %d, i8 %e);
