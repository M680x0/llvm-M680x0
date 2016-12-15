; RUN: llc < %s -mtriple=m680x0-linux | FileCheck %s --check-prefix=x00

; Function Attrs: norecurse nounwind readnone
; x00-LABEL c_isspace:
; x00: move.l #8388639, %d0
; x00: lsr.l %d1, %d0
; x00: and.l #1, %d0
define zeroext i1 @c_isspace(i32 %c) local_unnamed_addr #0 {
entry:
  %switch.tableidx = add i32 %c, -9
  %switch.cast = trunc i32 %switch.tableidx to i24
  %switch.downshift = lshr i24 -8388577, %switch.cast
  %0 = and i24 %switch.downshift, 1
  %switch.masked = icmp ne i24 %0, 0
  ret i1 %switch.masked
}
