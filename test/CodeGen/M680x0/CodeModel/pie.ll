; RUN: llc < %s -O0 -mtriple=m680x0-linux-gnu -relocation-model=pic -verify-machineinstrs | FileCheck %s -check-prefix=x00

define weak void @weak_foo() {
  ret void
}

define weak_odr void @weak_odr_foo() {
  ret void
}

define internal void @internal_foo() {
  ret void
}

declare i32 @ext_baz()

define void @foo() {
  ret void
}


; x00-LABEL: bar:
; x00:       jsr  (foo,%pc)
; x00:       jsr  (weak_odr_foo,%pc)
; x00:       jsr  (weak_foo,%pc)
; x00:       jsr  (internal_foo,%pc)
; x00:       jsr  (ext_baz@PLT,%pc)
define void @bar() {
entry:
  call void @foo()
  call void @weak_odr_foo()
  call void @weak_foo()
  call void @internal_foo()
  call i32 @ext_baz()
  ret void
}

; -fpie for local global data tests should be added here

!llvm.module.flags = !{!0, !1}
!0 = !{i32 1, !"PIC Level", i32 1}
!1 = !{i32 1, !"PIE Level", i32 1}
