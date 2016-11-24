; RUN: llc < %s -mtriple=m680x0-linux -verify-machineinstrs | FileCheck %s -check-prefix=x00

declare i32 @printf(i8*, ...) nounwind
declare {i32, i1} @llvm.smul.with.overflow.i32(i32, i32)
declare { i63, i1 } @llvm.smul.with.overflow.i63(i63, i63)

@ok = internal constant [4 x i8] c"%d\0A\00"
@no = internal constant [4 x i8] c"no\0A\00"

; x00-LABEL: test1:
; x00:       jsr __muldi3
; x00:       sne
define fastcc i1 @test1(i32 %v1, i32 %v2) nounwind {
entry:
  %t = call {i32, i1} @llvm.smul.with.overflow.i32(i32 %v1, i32 %v2)
  %sum = extractvalue {i32, i1} %t, 0
  %obit = extractvalue {i32, i1} %t, 1
  br i1 %obit, label %overflow, label %normal

normal:
  %t1 = tail call i32 (i8*, ...) @printf( i8* getelementptr ([4 x i8], [4 x i8]* @ok, i32 0, i32 0), i32 %sum ) nounwind
  ret i1 true

overflow:
  %t2 = tail call i32 (i8*, ...) @printf( i8* getelementptr ([4 x i8], [4 x i8]* @no, i32 0, i32 0) ) nounwind
  ret i1 false
}

; x00-LABEL: test2:
; x00:       jsr __muldi3
; x00:       sne
define fastcc i1 @test2(i32 %v1, i32 %v2) nounwind {
entry:
  %t = call {i32, i1} @llvm.smul.with.overflow.i32(i32 %v1, i32 %v2)
  %sum = extractvalue {i32, i1} %t, 0
  %obit = extractvalue {i32, i1} %t, 1
  br i1 %obit, label %overflow, label %normal

overflow:
  %t2 = tail call i32 (i8*, ...) @printf( i8* getelementptr ([4 x i8], [4 x i8]* @no, i32 0, i32 0) ) nounwind
  ret i1 false

normal:
  %t1 = tail call i32 (i8*, ...) @printf( i8* getelementptr ([4 x i8], [4 x i8]* @ok, i32 0, i32 0), i32 %sum ) nounwind
  ret i1 true
}

; x00-LABEL: test3:
; x00:       add.l
; x00-NEXT:  add.l
define i32 @test3(i32 %a, i32 %b) nounwind readnone {
entry:
	%tmp0 = add i32 %b, %a
	%tmp1 = call { i32, i1 } @llvm.smul.with.overflow.i32(i32 %tmp0, i32 2)
	%tmp2 = extractvalue { i32, i1 } %tmp1, 0
	ret i32 %tmp2
}

; x00-LABEL: test4:
; x00:       add.l
; x00:       jsr __muldi3
define i32 @test4(i32 %a, i32 %b) nounwind readnone {
entry:
	%tmp0 = add i32 %b, %a
	%tmp1 = call { i32, i1 } @llvm.smul.with.overflow.i32(i32 %tmp0, i32 4)
	%tmp2 = extractvalue { i32, i1 } %tmp1, 0
	ret i32 %tmp2
}
