; RUN: llc < %s -mtriple=m680x0-pc-linux -relocation-model=pic -verify-machineinstrs | FileCheck -check-prefix=x00 %s 

;
; C Call passes all arguments on stack ...

; x00-LABEL: test1:
; x00:       move.l (4,%sp), [[Reg:%a[0-7]]]
; x00:       move.l (8,%sp), ([[Reg]])
; x00:       rts
define void @test1(i32* nocapture %out, i32 %in) nounwind {
entry:
  store i32 %in, i32* %out, align 4
  ret void
}

; x00-LABEL: test2:
; x00-DAG:   move.l (4,%sp), [[Out:%a[0-7]]]
; x00-DAG:   move.l (8,%sp), [[In:%a[0-7]]]
; x00:       move.l ([[In]]), ([[Out]])
; x00:       rts
define void @test2(i32* nocapture %pOut, i32* nocapture %pIn) nounwind {
entry:
  %0 = load i32, i32* %pIn, align 4
  store i32 %0, i32* %pOut, align 4
  ret void
}

; x00-LABEL: test3:
; x00:       move.l (4,%sp), [[Reg:%a[0-7]]]
; x00:       move.b (11,%sp), ([[Reg]])
; x00:       rts
define void @test3(i8* nocapture %out, i8 %in) nounwind {
entry:
  store i8 %in, i8* %out, align 1
  ret void
}

; x00-LABEL: test4:
; x00:       move.l (4,%sp), [[Reg:%a[0-7]]]
; x00:       move.w (10,%sp), ([[Reg]])
; x00:       rts
define void @test4(i16* nocapture %out, i16 %in) nounwind {
entry:
  store i16 %in, i16* %out, align 2
  ret void
}

; x00-LABEL: test5:
; x00:       move.b (7,%sp), [[Reg:%d[0-7]]]
; x00:       add.b (11,%sp), [[Reg]]
; x00:       rts
define i8 @test5(i8 %a, i8 %b) nounwind {
entry:
  %add = add i8 %a, %b
  ret i8 %add
}

; x00-LABEL: test6:
; x00:       move.w (6,%sp), [[Reg:%d[0-7]]]
; x00:       add.w (10,%sp), [[Reg]]
; x00:       rts
define i16 @test6(i16 %a, i16 %b) nounwind {
entry:
  %add = add i16 %a, %b
  ret i16 %add
}
