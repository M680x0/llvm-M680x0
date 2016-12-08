; RUN: llc < %s -mtriple=m680x0-linux-gnu -verify-machineinstrs | FileCheck %s --check-prefix=x00

; x00-LABEL: test1:
; x00: cmpi.l #0, (%a0)
; x00: beq
define i32 @test1(i32* %y) nounwind {
 %tmp = load i32, i32* %y  ; <i32> [#uses=1]
 %tmp.upgrd.1 = icmp eq i32 %tmp, 0  ; <i1> [#uses=1]
 br i1 %tmp.upgrd.1, label %cond_true, label %cond_false

cond_false:  ; preds = %0
 ret i32 0

cond_true:     ; preds = %0
 ret i32 1
}

; x00-LABEL: test2:
; x00: and.l #536870911
; TODO There is no need for cmpi.l, and.l sets Z as well
; x00: cmpi.l #0
; x00: beq
define i32 @test2(i32* %y) nounwind {
 %tmp = load i32, i32* %y  ; <i32> [#uses=1]
 %tmp1 = shl i32 %tmp, 3  ; <i32> [#uses=1]
 %tmp1.upgrd.2 = icmp eq i32 %tmp1, 0  ; <i1> [#uses=1]
 br i1 %tmp1.upgrd.2, label %cond_true, label %cond_false

cond_false:  ; preds = %0
 ret i32 0

cond_true:  ; preds = %0
 ret i32 1
}

; x00-LABEL: test2b:
; x00: and.b #31
; x00: cmpi.b #0
; x00: beq
define i8 @test2b(i8* %y) nounwind {
 %tmp = load i8, i8* %y  ; <i8> [#uses=1]
 %tmp1 = shl i8 %tmp, 3  ; <i8> [#uses=1]
 %tmp1.upgrd.2 = icmp eq i8 %tmp1, 0  ; <i1> [#uses=1]
 br i1 %tmp1.upgrd.2, label %cond_true, label %cond_false

cond_false:  ; preds = %0
 ret i8 0

cond_true:  ; preds = %0
 ret i8 1
}

; x00-LABEL: test3:
; x00:       or.l
; x00-NEXT:  seq %d0
; x00-NEXT:  move.l %d0, %d1
; x00-NEXT:  and.l #255, %d1
; x00-NEXT:  move.l #0, %d0
define i64 @test3(i64 %x) nounwind {
  %t = icmp eq i64 %x, 0
  %r = zext i1 %t to i64
  ret i64 %r
}

; x00-LABEL: test4:
; x00:       sub.l #1
; x00:       subx.l
; x00:       slt
define i64 @test4(i64 %x) nounwind {
  %t = icmp slt i64 %x, 1
  %r = zext i1 %t to i64
  ret i64 %r
}

; x00-LABEL: test6:
; x00:       move.l (12,%sp), %d0
; x00:       or.l (8,%sp), %d0
; x00:       bne
; x00:       bra
define i32 @test6() nounwind align 2 {
  %A = alloca {i64, i64}, align 8
  %B = getelementptr inbounds {i64, i64}, {i64, i64}* %A, i64 0, i32 1
  %C = load i64, i64* %B
  %D = icmp eq i64 %C, 0
  br i1 %D, label %T, label %F
T:
  ret i32 1
  
F:
  ret i32 0
}

; x00-LABEL: test7:
; x00:       cmpi.l #0, (4,%sp)
; x00:       seq
define i32 @test7(i64 %res) nounwind {
entry:
  %lnot = icmp ult i64 %res, 4294967296
  %lnot.ext = zext i1 %lnot to i32
  ret i32 %lnot.ext
}

; x00-LABEL: test8:
; x00:       move.l (4,%sp), %d0
; x00:       sub.l #3, %d0
; x00:       scs %d0
define i32 @test8(i64 %res) nounwind {
entry:
  %lnot = icmp ult i64 %res, 12884901888
  %lnot.ext = zext i1 %lnot to i32
  ret i32 %lnot.ext
}

; x00-LABEL: test11:
; x00:       move.l (4,%sp), %d0
; x00:       and.l #-32768, %d0
; x00:       eori.l #32768, %d0
; x00:       seq
define i32 @test11(i64 %l) nounwind {
entry:
  %shr.mask = and i64 %l, -140737488355328
  %cmp = icmp eq i64 %shr.mask, 140737488355328
  %conv = zext i1 %cmp to i32
  ret i32 %conv
}

; x00-LABEL: test13:
; x00: move.b (7,%sp)
; x00: and.b #8
; x00: cmpi.b #0
define i32 @test13(i32 %mask, i32 %base, i32 %intra) {
  %and = and i32 %mask, 8
  %tobool = icmp ne i32 %and, 0
  %cond = select i1 %tobool, i32 %intra, i32 %base
  ret i32 %cond
}

; x00-LABEL: test14:
; x00:  move.l
; x00: lsr.l #7
; x00: cmpi.l #0
; x00: bpl
define i32 @test14(i32 %mask, i32 %base, i32 %intra) #0 {
  %s = lshr i32 %mask, 7
  %tobool = icmp sgt i32 %s, -1
  %cond = select i1 %tobool, i32 %intra, i32 %base
  ret i32 %cond
}

; x00-LABEL: test15:
; x00:  scc
; x00:  seq
; x00:  or.b
define zeroext i1 @test15(i32 %bf.load, i32 %n) {
  %bf.lshr = lshr i32 %bf.load, 16
  %cmp2 = icmp eq i32 %bf.lshr, 0
  %cmp5 = icmp uge i32 %bf.lshr, %n
  %.cmp5 = or i1 %cmp2, %cmp5
  ret i1 %.cmp5
}

; x00-LABEL: test16:
; x00:       move.w #15
; x00:       move.w (6,%sp), %d0
; x00:       lsr.w
; x00:       eori.b #1
define i8 @test16(i16 signext %L) {
  %lshr  = lshr i16 %L, 15
  %trunc = trunc i16 %lshr to i8
  %not   = xor i8 %trunc, 1
  ret i8 %not
}

; x00-LABEL: test18:
; x00:       move.l #31
; x00:       move.l
; x00:       lsr.l
; x00:       eori.b #1
define i8 @test18(i64 %L) {
  %lshr  = lshr i64 %L, 63
  %trunc = trunc i64 %lshr to i8
  %not   = xor i8 %trunc, 1
  ret i8 %not
}

@d = global i8 0, align 1


; x00-LABEL: test20
; x00: and.l
; x00: sne
; x00: add.l
; x00: sne
; x00: cmpi.l
; x00: sne
define void @test20(i32 %bf.load, i8 %x1, i8* %b_addr) {
  %bf.shl = shl i32 %bf.load, 8
  %bf.ashr = ashr exact i32 %bf.shl, 8
  %tobool4 = icmp ne i32 %bf.ashr, 0
  %conv = zext i1 %tobool4 to i32
  %conv6 = zext i8 %x1 to i32
  %add = add nuw nsw i32 %conv, %conv6
  %tobool7 = icmp ne i32 %add, 0
  %frombool = zext i1 %tobool7 to i8
  store i8 %frombool, i8* %b_addr, align 1
  %tobool14 = icmp ne i32 %bf.shl, 0
  %frombool15 = zext i1 %tobool14 to i8
  store i8 %frombool15, i8* @d, align 1
  ret void
}
