; RUN: llc < %s -mtriple=m680x0-pc-linux -relocation-model=pic -verify-machineinstrs | FileCheck -check-prefix=x00 %s 

;
; C Call passes all arguments on stack ...

; x00-LABEL: test1:
; x00:       move.l %d0, (%a0)
; x00-NEXT:  rts

define fastcc void @test1(i32* nocapture %out, i32 %in) nounwind {
entry:
  store i32 %in, i32* %out, align 4
  ret void
}

; x00-LABEL: test2:
; x00:       move.l (%a1), (%a0)
; x00-NEXT:  rts

define fastcc void @test2(i32* nocapture %pOut, i32* nocapture %pIn) nounwind {
entry:
  %0 = load i32, i32* %pIn, align 4
  store i32 %0, i32* %pOut, align 4
  ret void
}


; x00-LABEL: test3:
; x00:       add.l %d1, %d0
; x00-NEXT:  add.l %a0, %d0
; x00-NEXT:  add.l %a1, %d0
; x00-NEXT:  add.l (4,%sp), %d0

define fastcc i32 @test3(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e) nounwind {
  %1 = add i32 %a, %b
  %2 = add i32 %1, %c
  %3 = add i32 %2, %d
  %4 = add i32 %3, %e
  ret i32 %4
}
