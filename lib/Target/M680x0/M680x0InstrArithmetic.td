//===-- M680x0InstrArithmetic.td - Integer Arithmetic Instrs - tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the integer arithmetic instructions in the M680x0
// architecture. Here is the current status of the file:
//
//  Machine:
//
//    ADD       [~]   ADDA      [~]   ADDI        [~]   ADDQ [~]   ADDX [~]
//    CLR       [ ]   CMP       [ ]   CMPA        [ ]   CMPI [ ]   CMPM [ ]
//    CMP2      [ ]   DIVS/DIVU [ ]   DIVSL/DIVUL [ ]   EXT  [ ]   EXTB [ ]
//    MULS/MULU [ ]   NEG       [ ]   NEGX        [ ]   SUB  [ ]   SUBA [ ]
//    SUBI      [ ]   SUBQ      [ ]   SUBX        [ ]
//
//  Map:
//
//   [ ] - was not touched at all
//   [!] - requires extarnal stuff implemented
//   [~] - in progress but usable
//   [x] - done
//
//===----------------------------------------------------------------------===//

//FIXME extend to proper multiclasses
let isCommutable = 1 in { // FIXME is it really a commutable form?

// M680x0 instruction is dyadic where the second operand is destination
let Constraints = "$src = $dst" in {

// X — Set the same as the carry bit.
// N — Set if the result is negative; cleared otherwise.
// Z — Set if the result is zero; cleared otherwise.
// V — Set if an overflow is generated; cleared otherwise.
// C — Set if a carry is generated; cleared otherwise.
let Defs = [CCR] in {

// Dn <- Dn + Imm
def ADD8ri  : MxInst<0x000,
               (outs MxDRD8:$dst), (ins MxDRD8:$src, i8imm:$imm),
               "addi.b\t$imm, $dst",
               [(set i8:$dst, (add i8:$src, MximmSExt8:$imm))]>;

def ADD16ri : MxInst<0x000,
               (outs MxDRD16:$dst), (ins MxDRD16:$src, i16imm:$imm),
               "addi.w\t$imm, $dst",
               [(set i16:$dst, (add i16:$src, MximmSExt16:$imm))]>;

def ADD32ri : MxInst<0x000,
               (outs MxDRD32:$dst), (ins MxDRD32:$src, i32imm:$imm),
               "addi.l\t$imm, $dst",
               [(set i32:$dst, (add i32:$src, MximmSExt32:$imm))]>;

// Dn <- Dn + Dr
def ADD8rr  : MxInst<0x000,
               (outs MxDRD8:$dst), (ins MxDRD8:$src, MxDRD8:$reg),
               "add.b\t$reg, $dst",
               [(set i8:$dst, (add i8:$src, i8:$reg))]>;

def ADD16rr : MxInst<0x000,
               (outs MxDRD16:$dst), (ins MxDRD16:$src, MxDRD16:$reg),
               "add.w\t$reg, $dst",
               [(set i16:$dst, (add i16:$src, i16:$reg))]>;

def ADD32rr : MxInst<0x000,
               (outs MxDRD32:$dst), (ins MxDRD32:$src, MxDRD32:$reg),
               "add.l\t$reg, $dst",
               [(set i32:$dst, (add i32:$src, i32:$reg))]>;


// Dn <- Dn - Imm
def SUB8ri  : MxInst<0x000,
               (outs MxDRD8:$dst), (ins MxDRD8:$src, i8imm:$imm),
               "subi.b\t$imm, $dst",
               [(set i8:$dst, (sub i8:$src, MximmSExt8:$imm))]>;

def SUB16ri : MxInst<0x000,
               (outs MxDRD16:$dst), (ins MxDRD16:$src, i16imm:$imm),
               "subi.w\t$imm, $dst",
               [(set i16:$dst, (sub i16:$src, MximmSExt16:$imm))]>;

def SUB32ri : MxInst<0x000,
               (outs MxDRD32:$dst), (ins MxDRD32:$src, i32imm:$imm),
               "subi.l\t$imm, $dst",
               [(set i32:$dst, (sub i32:$src, MximmSExt32:$imm))]>;

// Dn <- Dn - Dr
def SUB8rr  : MxInst<0x000,
               (outs MxDRD8:$dst), (ins MxDRD8:$src, MxDRD8:$reg),
               "sub.b\t$reg, $dst",
               [(set i8:$dst, (sub i8:$src, i8:$reg))]>;

def SUB16rr : MxInst<0x000,
               (outs MxDRD16:$dst), (ins MxDRD16:$src, MxDRD16:$reg),
               "sub.w\t$reg, $dst",
               [(set i16:$dst, (sub i16:$src, i16:$reg))]>;

def SUB32rr : MxInst<0x000,
               (outs MxDRD32:$dst), (ins MxDRD32:$src, MxDRD32:$reg),
               "sub.l\t$reg, $dst",
               [(set i32:$dst, (sub i32:$src, i32:$reg))]>;
} // CCR Defs

// X — Not affected.
// N — Set if the most significant bit of the result is set; cleared otherwise.
// Z — Set if the result is zero; cleared otherwise.
// V — Always cleared.
// C — Always cleared.
let Defs = [CCR] in {

// Dn <- Dn AND Imm
def AND8ri  : MxInst<0x000,
               (outs MxDRD8:$dst), (ins MxDRD8:$src, i8imm:$imm),
               "andi.b\t$imm, $dst",
               [(set i8:$dst, (and i8:$src, MximmSExt8:$imm))]>;

def AND16ri : MxInst<0x000,
               (outs MxDRD16:$dst), (ins MxDRD16:$src, i16imm:$imm),
               "andi.w\t$imm, $dst",
               [(set i16:$dst, (and i16:$src, MximmSExt16:$imm))]>;

def AND32ri : MxInst<0x000,
               (outs MxDRD32:$dst), (ins MxDRD32:$src, i32imm:$imm),
               "andi.l\t$imm, $dst",
               [(set i32:$dst, (and i32:$src, MximmSExt32:$imm))]>;

// Dn <- Dn AND Dr
def AND8rr  : MxInst<0x000,
               (outs MxDRD8:$dst), (ins MxDRD8:$src, MxDRD8:$reg),
               "and.b\t$reg, $dst",
               [(set i8:$dst, (and i8:$src, i8:$reg))]>;

def AND16rr : MxInst<0x000,
               (outs MxDRD16:$dst), (ins MxDRD16:$src, MxDRD16:$reg),
               "and.w\t$reg, $dst",
               [(set i16:$dst, (and i16:$src, i16:$reg))]>;

def AND32rr : MxInst<0x000,
               (outs MxDRD32:$dst), (ins MxDRD32:$src, MxDRD32:$reg),
               "and.l\t$reg, $dst",
               [(set i32:$dst, (and i32:$src, i32:$reg))]>;
} // CCR Defs
} // Constraints
} // isCommutable
