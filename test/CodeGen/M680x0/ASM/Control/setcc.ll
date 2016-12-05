; RUN: llc < %s -mtriple=m680x0-linux -verify-machineinstrs | FileCheck %s -check-prefix=x00

;; TODO All these can be improved

; x00-LABEL: t1:
; x00:       move.w
; x00:       and.l #65535, %d0
; x00:       sub.l #26, %d0
; x00:       shi %d0
; x00:       and.l #255, %d0
; x00:       lsl.l #5, %d0

define zeroext i16 @t1(i16 zeroext %x) nounwind readnone ssp {
entry:
  %0 = icmp ugt i16 %x, 26                        ; <i1> [#uses=1]
  %iftmp.1.0 = select i1 %0, i16 32, i16 0        ; <i16> [#uses=1]
  ret i16 %iftmp.1.0
}


; x00-LABEL: t2:
; x00:       move.w
; x00:       and.l #65535, %d0
; x00:       sub.l #26, %d0
; x00:       scs %d0
; x00:       and.l #255, %d0
; x00:       lsl.l #5, %d0

define zeroext i16 @t2(i16 zeroext %x) nounwind readnone ssp {
entry:
  %0 = icmp ult i16 %x, 26                        ; <i1> [#uses=1]
  %iftmp.0.0 = select i1 %0, i16 32, i16 0        ; <i16> [#uses=1]
  ret i16 %iftmp.0.0
}

; x00-LABEL: t3:
; x00:       move.l #0, %d2
; x00:       sub.l #18, %d1
; x00:       subx.l %d2, %d0
; x00:       scs
; x00:       and.l
; x00:       lsl.l

define fastcc i64 @t3(i64 %x) nounwind readnone ssp {
entry:
  %0 = icmp ult i64 %x, 18                        ; <i1> [#uses=1]
  %iftmp.2.0 = select i1 %0, i64 64, i64 0        ; <i64> [#uses=1]
  ret i64 %iftmp.2.0
}

@v4 = common global i32 0, align 4

; x00-LABEL: t4:
; x00:       cmpi.l #1, (v4,%pc)
; x00:       subx.w %d0, %d0
; x00:       lsr.w
; x00:       add.w
; x00:       lsl.l

define i32 @t4(i32 %a) {
entry:
  %0 = load i32, i32* @v4, align 4
  %not.tobool = icmp eq i32 %0, 0
  %conv.i = sext i1 %not.tobool to i16
  %call.lobit = lshr i16 %conv.i, 15
  %add.i.1 = add nuw nsw i16 %call.lobit, 1
  %conv4.2 = zext i16 %add.i.1 to i32
  %add = shl nuw nsw i32 %conv4.2, 16
  ret i32 %add
}


; x00-LABEL: t5:
; x00:       move.l #31
; x00:       move.l (4
; x00:       lsr.l
; x00:       eori.b
;
; Should be:
; cmp.l
; smi
; since we are intereseted in sign bit only

define i8 @t5(i32 %a) {
entry:
  %.lobit = lshr i32 %a, 31
  %trunc = trunc i32 %.lobit to i8
  %.not = xor i8 %trunc, 1
  ret i8 %.not
}


; x00-LABEL: t6:
; x00:       move.l #31
; x00:       move.l (4
; x00:       lsr.l
; x00:       eori.b
; x00:       and.l
;
; Should be:
; cmp.l
; smi
; since we are intereseted in sign bit only
; and.l in the end is superfluous

define zeroext i1 @t6(i32 %a) {
entry:
  %.lobit = lshr i32 %a, 31
  %trunc = trunc i32 %.lobit to i1
  %.not = xor i1 %trunc, 1
  ret i1 %.not
}
