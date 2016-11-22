; RUN: llc < %s -mtriple=m680x0-linux | FileCheck %s --check-prefix=x00

; x00-LABEL: mul4_32:
; x00:       lsl.l
define i32 @mul4_32(i32 %A) {
    %mul = mul i32 %A, 4
    ret i32 %mul
}

; x00-LABEL: mul4_64:
; x00:       lsr.l
; x00:       lsl.l
; x00:       or.l
; x00:       lsl.l
define i64 @mul4_64(i64 %A) {
    %mul = mul i64 %A, 4
    ret i64 %mul
}

; x00-LABEL: mul4096_32:
; x00:       lsl.l
define i32 @mul4096_32(i32 %A) {
    %mul = mul i32 %A, 4096
    ret i32 %mul
}

; x00-LABEL: mul4096_64:
; x00:       lsr.l
; x00:       lsl.l
; x00:       or.l
; x00:       lsl.l
define i64 @mul4096_64(i64 %A) {
    %mul = mul i64 %A, 4096
    ret i64 %mul
}

; x00-LABEL: mulmin4096_32:
; x00:       lsl.l
; x00-NEXT:  neg.l
define i32 @mulmin4096_32(i32 %A) {
    %mul = mul i32 %A, -4096
    ret i32 %mul
}

; x00-LABEL: mulmin4096_64:
; x00:       lsr.l
; x00:       lsl.l
; x00:       or.l
; x00:       lsl.l
; x00:       neg.l
; x00:       negx.l
define i64 @mulmin4096_64(i64 %A) {
    %mul = mul i64 %A, -4096
    ret i64 %mul
}

; No i32 multiply for M68000
; x00-LABEL: mul3_32:
; x00:       jsr __mulsi3
define i32 @mul3_32(i32 %A) {
    %mul = mul i32 %A, 3
    ret i32 %mul
}

; x00-LABEL: mul3_64:
; x00:       jsr __muldi3
define i64 @mul3_64(i64 %A) {
    %mul = mul i64 %A, 3
    ret i64 %mul
}

; x00-LABEL: mul40_32:
; x00:       jsr __mulsi3
define i32 @mul40_32(i32 %A) {
    %mul = mul i32 %A, 40
    ret i32 %mul
}

; x00-LABEL: mul40_64:
; x00:       jsr __muldi3
define i64 @mul40_64(i64 %A) {
    %mul = mul i64 %A, 40
    ret i64 %mul
}

; x00-LABEL: mul4_32_minsize:
; x00:       lsl.l
define i32 @mul4_32_minsize(i32 %A) minsize {
    %mul = mul i32 %A, 4
    ret i32 %mul
}

; X64-LABEL: mul40_32_minsize:
; X64:       jsr __mulsi3
define i32 @mul40_32_minsize(i32 %A) minsize {
    %mul = mul i32 %A, 40
    ret i32 %mul
}

; x00-LABEL: mul33_32:
; x00:       __mulsi3
define i32 @mul33_32(i32 %A) {
    %mul = mul i32 %A, 33
    ret i32 %mul
}

; x00-LABEL: mul31_32:
; x00:       __mulsi3
define i32 @mul31_32(i32 %A) {
    %mul = mul i32 %A, 31
    ret i32 %mul
}

; x00-LABEL: mul0_32:
; x00:       move.l #0
define i32 @mul0_32(i32 %A) {
    %mul = mul i32 %A, 0
    ret i32 %mul
}

; x00-LABEL: mul4294967295_32:
; x00:       move.l #0
; x00:       sub.l
define i32 @mul4294967295_32(i32 %A) {
    %mul = mul i32 %A, 4294967295
    ret i32 %mul
}

; x00-LABEL: mul18446744073709551615_64:
; x00:       move.l #0
; x00-NEXT:  sub.l
; x00-NEXT:  move.l
; x00-NEXT:  negx.l
define i64 @mul18446744073709551615_64(i64 %A) {
    %mul = mul i64 %A, 18446744073709551615
    ret i64 %mul
}
