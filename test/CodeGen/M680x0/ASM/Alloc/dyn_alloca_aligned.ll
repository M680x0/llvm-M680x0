; RUN: llc < %s -mtriple=m68k-linux-gnu -verify-machineinstrs | FileCheck %s -check-prefix=x00
define i32 @A(i32 %Size) {
; x00:       sub.l %d1, %d0
; x00-NEXT:  and.l #-128
; x00-NEXT:  move.l %d0, %sp
  %A = alloca i8, i32 %Size, align 128
  %A_addr = ptrtoint i8* %A to i32
  ret i32 %A_addr
}
