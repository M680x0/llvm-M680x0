; RUN: llc < %s -O2 -mtriple=m680x0-linux-gnu -verify-machineinstrs \
; RUN:              -code-model=medium -relocation-model=pic        \
; RUN: | FileCheck %s -check-prefix=x00

; External Linkage
@a = global i32 0, align 4

; x00-LABEL: my_access_global_a:
; x00:       lea (_GLOBAL_OFFSET_TABLE_@GOTPC,%pc), %a0
; x00:       move.l (a@GOTOFF,%a0), %d0
; x00-NEXT:  rts
define i32 @my_access_global_a() #0 {
entry:
  %0 = load i32, i32* @a, align 4
  ret i32 %0
}

; WeakAny Linkage
@b = weak global i32 0, align 4

; x00-LABEL: my_access_global_b:
; x00:       lea (_GLOBAL_OFFSET_TABLE_@GOTPC,%pc), %a0
; x00-NEXT:  move.l (b@GOTOFF,%a0), %d0
define i32 @my_access_global_b() #0 {
entry:
 %0 = load i32, i32* @b, align 4
 ret i32 %0
}

; Internal Linkage
@c = internal global i32 0, align 4

; x00-LABEL: my_access_global_c:
; x00:       lea (_GLOBAL_OFFSET_TABLE_@GOTPC,%pc), %a0
; x00-NEXT:  move.l (c@GOTOFF,%a0), %d0
define i32 @my_access_global_c() #0 {
entry:
 %0 = load i32, i32* @c, align 4
 ret i32 %0
}

; External Linkage, only declaration.
@d = external global i32, align 4

; x00-LABEL: my_access_global_load_d:
; x00:       move.l (d@GOTPCREL,%pc), %a0
; x00-NEXT:  move.l (%a0), %d0
define i32 @my_access_global_load_d() #0 {
entry:
 %0 = load i32, i32* @d, align 4
 ret i32 %0
}

; External Linkage, only declaration, store a value.
; x00-LABEL: my_access_global_store_d:
; x00:       move.l (d@GOTPCREL,%pc), %a0
; x00-NEXT:  move.l #2, (%a0)
define i32 @my_access_global_store_d() #0 {
entry:
 store i32 2, i32* @d, align 4
 ret i32 0
}

; External Linkage, function pointer access.
declare i32 @access_fp(i32 ()*)
declare i32 @foo()

; x00-LABEL: my_access_fp_foo:
; x00:       move.l (foo@GOTPCREL,%pc), (%sp)
; x00-NEXT:  jsr (access_fp@PLT,%pc)
define i32 @my_access_fp_foo() #0 {
entry:
 %call = call i32 @access_fp(i32 ()* @foo)
 ret i32 %call
}

; LinkOnceODR Linkage, function pointer access.

$bar = comdat any

define linkonce_odr i32 @bar() comdat {
entry:
 ret i32 0
}

; x00-LABEL: my_access_fp_bar:
; x00:       lea (_GLOBAL_OFFSET_TABLE_@GOTPC,%pc), %a0
; x00-NEXT:  lea (bar@GOTOFF,%a0), %a0
; x00-NEXT:  move.l %a0, (%sp)
; x00-NEXT:  jsr (access_fp@PLT,%pc)
define i32 @my_access_fp_bar() #0 {
entry:
 %call = call i32 @access_fp(i32 ()* @bar)
 ret i32 %call
}

!llvm.module.flags = !{!0, !1}
!0 = !{i32 1, !"PIC Level", i32 1}
!1 = !{i32 1, !"PIE Level", i32 1}
