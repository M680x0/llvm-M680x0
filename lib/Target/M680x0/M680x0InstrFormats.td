//=== M680x0InstrFormats.td - Motorola 680x0 Instruction Formats tablegen -===//
// ADDRESS REGISTER INDIRECT WITH DISPLACEMENT. This addressing mode requires one
// word of extension. The address of the operand is the sum of the address in
// the address register and the sign-extended 16-bit displacement integer in the
// extension word. The reference is classified as a data reference with the
// exception of the jump and jump-to-subroutine instructions.
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Since M680x0 has quite a lot memory addressing modes there are more
// instruction prefixes than just i, r and m:
// TSF  Since     Form                     Letter  Description
//  00   M68000    Dn or An                 r       any register
//  01   M68000    Dn                       d       data register direct
//  02   M68000    An                       a       address register direct
//  03   M68000    (An)                     m       address register indirect
//  04   M68000    (An)+                    o       address register indirect with postincrement
//  05   M68000    -(An)                    e       address register indirect with predecrement
//  06   M68000    (i,An)                   p       address register indirect with displacement
//  07   M68000    (i,An,Xn.W)              f       address register indirect with index and scale = 1
//  10   M68000    (i,An,Xn.L)              F       address register indirect with index and scale = 1
//  11   M68020    (i,An,Xn.W,SCALE)        g       address register indirect with index
//  12   M68020    (i,An,Xn.L,SCALE)        G       address register indirect with index
//  13   M68020    ([bd,An],Xn.W,SCALE,od)  u       memory indirect postindexed mode
//  14   M68020    ([bd,An],Xn.L,SCALE,od)  U       memory indirect postindexed mode
//  15   M68020    ([bd,An,Xn.W,SCALE],od)  v       memory indirect preindexed mode
//  16   M68020    ([bd,An,Xn.L,SCALE],od)  V       memory indirect preindexed mode
//  17   M68000    abs.W                    b       absolute short address
//  20   M68000    abs.L                    B       absolute long address
//  21   M68000    (i,PC)                   q       program counter with displacement
//  22   M68000    (i,PC,Xn.W)              k       program counter with index and scale = 1
//  23   M68000    (i,PC,Xn.L)              K       program counter with index and scale = 1
//  24   M68020    (i,PC,Xn.W,SCALE)        l       program counter with index
//  25   M68020    (i,PC,Xn.L,SCALE)        L       program counter with index
//  26   M68020    ([bd,PC],Xn.W,SCALE,od)  x       program counter memory indirect postindexed mode
//  27   M68020    ([bd,PC],Xn.L,SCALE,od)  X       program counter memory indirect postindexed mode
//  30   M68020    ([bd,PC,Xn.W,SCALE],od)  y       program counter memory indirect preindexed mode
//  31   M68020    ([bd,PC,Xn.L,SCALE],od)  Y       program counter memory indirect preindexed mode
//  32   M68000    #immediate               i       immediate data
//
// Operand can be quilified with size where appropriate to force a particular
// instruction encoding, e.g.:
//    (i8,An,Rn)               x8      1 extension word
//    (i16,An,Rn)              x16     2 extension words
//    (i32,An,Rn)              x32     3 extension words
//
// Form without size qualifier will adapt to operand size automatically, e.g.:
//    (i,An,Rn)                x       1, 2 or 3 extension words
//
// Some forms alreay imply a particular size of their operands, e.g.:
//    (i,An)                   j       1 extension word and i is 16bit
//
// Operand order follows x86 Intel order(destination before source), e.g.:
//    MOV8dx                   MOVE (4,A0,D0), D1
//
// Number after instruction mnemonics determines the size of the data
//
//===----------------------------------------------------------------------===//

// M680x0 INSTRUCTION. Most instructions specify the location of an operand by
// using the effective address field in the operation word. The effective address
// is composed of two 3-bit fields: the mode field and the register field. The
// value in the mode field selects the different address modes. The register
// field contains the number of a register.  The effective address field may
// require additional information to fully specify the operand. This additional
// information, called the effective address extension, is contained in the
// following word or words and is considered part of the instruction. The
// effective address modes are grouped into three categories: register direct,
// memory addressing, and special.
class MxInst<bits<16> opcode,
             dag outs, dag ins,
             string asmStr = "",
             list<dag> pattern = [],
             InstrItinClass itin = NoItinerary>
    : Instruction {
  let Namespace      = "M680x0";
  let OutOperandList = outs;
  let InOperandList  = ins;
  let AsmString      = asmStr;
  let Pattern        = pattern;
  let Itinerary      = itin;

  // NOTE Opcode is no really applicable for the most instructions since they are
  // composed based on the actual register used and EA mode
  bits<16> Opcode = opcode;
}

// M680x0 PSEUDO INSTRUCTION
class MxPseudo<dag outs, dag ins, string asmStr = "", list<dag> pattern = []>
    : MxInst<0x0, outs, ins, asmStr, pattern> {
  let isPseudo = 1;
}
