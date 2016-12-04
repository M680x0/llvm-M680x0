; RUN: llc < %s -mtriple=m680x0-pc-linux -relocation-model=pic -verify-machineinstrs | FileCheck -check-prefix=x00 %s 

;
; C Call passes all arguments on stack ...

; x00-LABEL: foo:
; x00:       move.l (4,%sp), [[Reg:%a[0-7]]]
; x00:       move.l (8,%sp), ([[Reg]])
; x00:       rts

define void @foo(i32* nocapture %out, i32 %in) nounwind {
entry:
  store i32 %in, i32* %out, align 4
  ret void
}

; x00-LABEL: bar:
; x00-DAG:   move.l (4,%sp), [[Out:%a[0-7]]]
; x00-DAG:   move.l (8,%sp), [[In:%a[0-7]]]
; x00:       move.l ([[In]]), ([[Out]])
; x00:       rts

define void @bar(i32* nocapture %pOut, i32* nocapture %pIn) nounwind {
entry:
  %0 = load i32, i32* %pIn, align 4
  store i32 %0, i32* %pOut, align 4
  ret void
}
