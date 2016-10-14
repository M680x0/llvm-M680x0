; RUN: llc < %s -mtriple=m680x0-pc-linux -relocation-model=pic -verify-machineinstrs | FileCheck -check-prefix=x00 %s 

;
; Pass first two arguments in registers %d0 and %d1

; x00-LABEL: foo_inreg:
; x00:       move.l %d0, [[Reg:%a[0-7]]]
; x00:       move.l %d1, ([[Reg]])
; x00:       rts

define void @foo_inreg(i32* nocapture inreg %out, i32 inreg %in) nounwind {
entry:
  store i32 %in, i32* %out, align 4
  ret void
}

; x00-LABEL: bar_inreg:
; x00-DAG:   move.l %d0, [[Out:%a[0-7]]]
; x00-DAG:   move.l %d1, [[In:%a[0-7]]]
; x00:       move.l ([[In]]), ([[Out]])
; x00:       rts

define void @bar_inreg(i32* nocapture inreg %pOut, i32* nocapture inreg %pIn) nounwind {
entry:
  %0 = load i32, i32* %pIn, align 4
  store i32 %0, i32* %pOut, align 4
  ret void
}
