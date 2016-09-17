//== M680x0InstrInfo.td - Main Motorola 680x0 Instruction Definition tablegen =//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the M680x0 instruction set, defining the instructions and
// properties of the instructions which are needed for code generation, machine
// code emission, and analysis.
//
//===----------------------------------------------------------------------===//

include "M680x0InstrFormats.td"

//===----------------------------------------------------------------------===//
// Profiles
//===----------------------------------------------------------------------===//

def SDTM680x0Ret : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;

//===----------------------------------------------------------------------===//
// Nodes
//===----------------------------------------------------------------------===//

def M680x0Ret : SDNode<"M680x0ISD::RET", SDTM680x0Ret,
                       [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

//===----------------------------------------------------------------------===//
// Operands
//===----------------------------------------------------------------------===//

// DATA REGISTER DIRECT. The operand is in the data register specified by
// the effective address register field.
def M680x0_DRD8  : RegisterOperand<DR8,  "printOperand">;
def M680x0_DRD16 : RegisterOperand<DR16, "printOperand">;
def M680x0_DRD32 : RegisterOperand<DR32, "printOperand">;

// ADDRESS REGISTER DIRECT. The operand is in the address register specified by
// the effective address register field.
def M680x0_ARD : RegisterOperand<AR32, "printARDOperand">;

// TODO finish parser wiring
def M680x0MemAsmOperand : AsmOperandClass {
 let Name = "Mem";
}

class M680x0MemOperand<dag ops, string printMethod,
          AsmOperandClass parserMatchClass = M680x0MemAsmOperand>
    : Operand<iPTR> {
  let PrintMethod = printMethod;
  let MIOperandInfo = ops;
  let ParserMatchClass = parserMatchClass;
  let OperandType = "OPERAND_MEMORY";
}

// ADDRESS REGISTER INDIRECT. The address of the operand is in the address
// register specified by the register field. The reference is classified as
// a data reference with the exception of the jump and jump-to-subroutine
// instructions.
def M680x0_ARI8     : M680x0MemOperand<(ops AR32), "printARI8Mem">;
def M680x0_ARI16    : M680x0MemOperand<(ops AR32), "printARI16Mem">;
def M680x0_ARI32    : M680x0MemOperand<(ops AR32), "printARI32Mem">;

// ADDRESS REGISTER INDIRECT WITH POSTINCREMENT. The address of the operand is
// in the address register specified by the register field. After the operand
// address is used, it is incremented by one, two, or four depending upon whether
// the size of the operand is byte, word, or long word. If the address register
// is the stack pointer and the operand size is byte, the address is incremented
// by two rather than one to keep the stack pointer on a word boundary.
// The reference is classified as a data reference.
def M680x0_ARIPI8   : M680x0MemOperand<(ops AR32), "printARIPI8Mem">;
def M680x0_ARIPI16  : M680x0MemOperand<(ops AR32), "printARIPI16Mem">;
def M680x0_ARIPI32  : M680x0MemOperand<(ops AR32), "printARIPI32Mem">;

// ADDRESS REGISTER INDIRECT WITH PREDECREMENT. The address of the operand is in
// the address register specified by the register field. Before the operand
// address is used, it is decremented by one, two, or four depending upon whether
// the operand size is byte, word, or long word. If the address register is
// the stack pointer and the operand size is byte, the address is decremented by
// two rather than one to keep the stack pointer on a word boundary.
// The reference is classified as a data reference.
def M680x0_ARIPD8   : M680x0MemOperand<(ops AR32), "printARIPD8Mem">;
def M680x0_ARIPD16  : M680x0MemOperand<(ops AR32), "printARIPD16Mem">;
def M680x0_ARIPD32  : M680x0MemOperand<(ops AR32), "printARIPD32Mem">;

// ADDRESS REGISTER INDIRECT WITH DISPLACEMENT. This addressing mode requires one
// word of extension. The address of the operand is the sum of the address in
// the address register and the sign-extended 16-bit displacement integer in the
// extension word. The reference is classified as a data reference with the
// exception of the jump and jump-to-subroutine instructions.
def M680x0_ARID8    : M680x0MemOperand<(ops i16imm, AR32), "printARID8Mem">;
def M680x0_ARID16   : M680x0MemOperand<(ops i16imm, AR32), "printARID16Mem">;
def M680x0_ARID32   : M680x0MemOperand<(ops i16imm, AR32), "printARID32Mem">;

// ADDRESS REGISTER INDIRECT WITH INDEX. This addressing mode requires one word
// of extension. The address of the operand is the sum of the address in the
// address register, the signextended displacement integer in the low order eight
// bits of the extension word, and the contents of the index register.
// The reference is classified as a data reference with the exception of the
// jump and jump-to-subroutine instructions
def M680x0_ARII8w   : M680x0MemOperand<(ops i8imm, AR32, IR16), "printARII8wMem">;
def M680x0_ARII16w  : M680x0MemOperand<(ops i8imm, AR32, IR16), "printARII16wMem">;
def M680x0_ARII32w  : M680x0MemOperand<(ops i8imm, AR32, IR16), "printARII32wMem">;

def M680x0_ARII8l   : M680x0MemOperand<(ops i8imm, AR32, IR32), "printARII8lMem">;
def M680x0_ARII16l  : M680x0MemOperand<(ops i8imm, AR32, IR32), "printARII16lMem">;
def M680x0_ARII32l  : M680x0MemOperand<(ops i8imm, AR32, IR32), "printARII32lMem">;

// ABSOLUTE SHORT ADDRESS. This addressing mode requires one word of extension.
// The address of the operand is the extension word. The 16-bit address is sign
// extended before it is used.  The reference is classified as a data reference
// with the exception of the jump and jump-tosubroutine instructions.
def M680x0_AS8      : M680x0MemOperand<(ops i16imm), "printAS8Mem">;
def M680x0_AS16     : M680x0MemOperand<(ops i16imm), "printAS16Mem">;
def M680x0_AS32     : M680x0MemOperand<(ops i16imm), "printAS32Mem">;

// ABSOLUTE LONG ADDRESS. This addressing mode requires two words of extension.
// The address of the operand is developed by the concatenation of the extension
// words. The high order part of the address is the first extension word; the low
// order part of the address is the second extension word. The reference is
// classified as a data reference with the exception of the jump and jump
// to-subroutine instructions.
def M680x0_AL8      : M680x0MemOperand<(ops i32imm), "printAL8Mem">;
def M680x0_AL16     : M680x0MemOperand<(ops i32imm), "printAL16Mem">;
def M680x0_AL32     : M680x0MemOperand<(ops i32imm), "printAL32Mem">;

let OperandType = "OPERAND_PCREL" in {
// PROGRAM COUNTER WITH DISPLACEMENT. This addressing mode requires one word of
// extension. The address of the operand is the sum of the address in the program
// counter and the Sign-extended 16-bit displacement integer in the extension
// word. The value in the program counter is the address of the extension word.
// The reference is classified as a program reference.
def M680x0_PCD8     : M680x0MemOperand<(ops i16imm), "printPCD8Mem">;
def M680x0_PCD16    : M680x0MemOperand<(ops i16imm), "printPCD16Mem">;
def M680x0_PCD32    : M680x0MemOperand<(ops i16imm), "printPCD32Mem">;

// PROGRAM COUNTER WITH INDEX. This addressing mode requires one word of
// extension. The address is the sum of the address in the program counter, the
// sign-extended displacement integer in the lower eight bits of the extension
// word, and the contents of the index register.  The value in the program
// counter is the address of the extension word. This reference is classified as
// a program reference.
def M680x0_PCI8w   : M680x0MemOperand<(ops i8imm, IR16), "printPCI8wMem">;
def M680x0_PCI16w  : M680x0MemOperand<(ops i8imm, IR16), "printPCI16wMem">;
def M680x0_PCI32w  : M680x0MemOperand<(ops i8imm, IR16), "printPCI32wMem">;

def M680x0_PCI8l   : M680x0MemOperand<(ops i8imm, IR32), "printPCI8lMem">;
def M680x0_PCI16l  : M680x0MemOperand<(ops i8imm, IR32), "printPCI16lMem">;
def M680x0_PCI32l  : M680x0MemOperand<(ops i8imm, IR32), "printPCI32lMem">;
} // let OperandType

let OperandType = "OPERAND_IMMEDIATE" in {
// IMMEDIATE DATA. This addressing mode requires either one or two words of
// extension depending on the size of the operation.
//     Byte Operation - operand is low order byte of extension word
//     Word Operation - operand is extension word
//     Long Word Operation - operand is in the two extension words,
//                           high order 16 bits are in the first
//                           extension word, low order 16 bits are
//                           in the second extension word.
def M680x0_Imm8    : Operand<i8>;
def M680x0_Imm16   : Operand<i16>;
def M680x0_Imm32   : Operand<i32>;
} // let OperandType

//===----------------------------------------------------------------------===//
// Complex Patterns
//===----------------------------------------------------------------------===//

def M680x0CP_ARIPI : ComplexPattern<iPTR, 1, "selectARIPI",
                                    [frameindex], [SDNPWantParent]>;

def M680x0CP_ARIPD : ComplexPattern<iPTR, 1, "selectARIPD",
                                    [frameindex], [SDNPWantParent]>;

def M680x0CP_ARID  : ComplexPattern<iPTR, 2, "selectARID",
                                    [frameindex], [SDNPWantParent]>;

def M680x0CP_ARII  : ComplexPattern<iPTR, 3, "selectARII",
                                    [frameindex], [SDNPWantParent]>;

def M680x0CP_PCD   : ComplexPattern<iPTR, 1, "selectPCD",
                                    [frameindex], [SDNPWantParent]>;

def M680x0CP_PCI   : ComplexPattern<iPTR, 2, "selectPCI",
                                    [frameindex], [SDNPWantParent]>;

//===----------------------------------------------------------------------===//
// Predicates
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Pattern Fragments
//===----------------------------------------------------------------------===//

// Helper fragments for loads.
// It's always safe to treat a anyext i16 load as a i32 load if the i16 is
// known to be 32-bit aligned or better. Ditto for i8 to i16.
def M680x0_loadi16 : PatFrag<(ops node:$ptr), (i16 (unindexedload node:$ptr)), [{
  LoadSDNode *LD = cast<LoadSDNode>(N);
  ISD::LoadExtType ExtType = LD->getExtensionType();
  if (ExtType == ISD::NON_EXTLOAD)
    return true;
  if (ExtType == ISD::EXTLOAD)
    return LD->getAlignment() >= 2 && !LD->isVolatile();
  return false;
}]>;

def M680x0_loadi32 : PatFrag<(ops node:$ptr), (i32 (unindexedload node:$ptr)), [{
  LoadSDNode *LD = cast<LoadSDNode>(N);
  ISD::LoadExtType ExtType = LD->getExtensionType();
  if (ExtType == ISD::NON_EXTLOAD)
    return true;
  if (ExtType == ISD::EXTLOAD)
    return LD->getAlignment() >= 4 && !LD->isVolatile();
  return false;
}]>;

def M680x0_loadi8         : PatFrag<(ops node:$ptr), (i8  (load node:$ptr))>;

def M680x0_sextloadi16i8  : PatFrag<(ops node:$ptr), (i16 (sextloadi8 node:$ptr))>;
def M680x0_sextloadi32i8  : PatFrag<(ops node:$ptr), (i32 (sextloadi8 node:$ptr))>;
def M680x0_sextloadi32i16 : PatFrag<(ops node:$ptr), (i32 (sextloadi16 node:$ptr))>;

def M680x0_zextloadi8i1   : PatFrag<(ops node:$ptr), (i8  (zextloadi1 node:$ptr))>;
def M680x0_zextloadi16i1  : PatFrag<(ops node:$ptr), (i16 (zextloadi1 node:$ptr))>;
def M680x0_zextloadi32i1  : PatFrag<(ops node:$ptr), (i32 (zextloadi1 node:$ptr))>;
def M680x0_zextloadi16i8  : PatFrag<(ops node:$ptr), (i16 (zextloadi8 node:$ptr))>;
def M680x0_zextloadi32i8  : PatFrag<(ops node:$ptr), (i32 (zextloadi8 node:$ptr))>;
def M680x0_zextloadi32i16 : PatFrag<(ops node:$ptr), (i32 (zextloadi16 node:$ptr))>;

def M680x0_extloadi8i1    : PatFrag<(ops node:$ptr), (i8  (extloadi1 node:$ptr))>;
def M680x0_extloadi16i1   : PatFrag<(ops node:$ptr), (i16 (extloadi1 node:$ptr))>;
def M680x0_extloadi32i1   : PatFrag<(ops node:$ptr), (i32 (extloadi1 node:$ptr))>;
def M680x0_extloadi16i8   : PatFrag<(ops node:$ptr), (i16 (extloadi8 node:$ptr))>;
def M680x0_extloadi32i8   : PatFrag<(ops node:$ptr), (i32 (extloadi8 node:$ptr))>;
def M680x0_extloadi32i16  : PatFrag<(ops node:$ptr), (i32 (extloadi16 node:$ptr))>;

def immSExt8  : ImmLeaf<i8,  [{ return isInt<8>(Imm);  }]>;
def immSExt16 : ImmLeaf<i16, [{ return isInt<16>(Imm); }]>;
def immSExt32 : ImmLeaf<i32, [{ return isInt<32>(Imm); }]>;

//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

// TODO make instr set CCR like X86 does
// Do i need this?

// NOP
let hasSideEffects = 0 in {
  def NOP : M680x0Inst<0x4E71, (outs), (ins), "nop", []>;
}

// FIXME these must conform to M680x0 EA operands described above

// MOVE
// X - not affected
// N,Z - set by operation
// V,C - cleared
let Defs = [CCR] in {

// Dn <- Dn
def MOV8rr  : M680x0Inst<0x1000, (outs M680x0_DRD8:$dst), (ins M680x0_DRD8:$src),
                         "move.b\t$src, $dst",
                         []>;

def MOV16rr : M680x0Inst<0x3000, (outs M680x0_DRD16:$dst), (ins M680x0_DRD16:$src),
                         "move.w\t$src, $dst",
                         []>;

def MOV32rr : M680x0Inst<0x2000, (outs M680x0_DRD32:$dst), (ins M680x0_DRD32:$src),
                         "move.l\t$src, $dst",
                         []>;


// Dn <- Imm
// TODO what the hell this means?
let isReMaterializable = 1, isAsCheapAsAMove = 1 in {
def MOV8ri  : M680x0Inst<0x1000, (outs DR8 :$dst), (ins i8imm :$src),
                         "move.b\t$src, $dst",
                         [(set i8:$dst, immSExt8:$src)]>;

def MOV16ri : M680x0Inst<0x3000, (outs DR16:$dst), (ins i16imm:$src),
                         "move.w\t$src, $dst",
                         [(set i16:$dst, immSExt16:$src)]>;

def MOV32ri : M680x0Inst<0x2000, (outs DR32:$dst), (ins i32imm:$src),
                         "move.l\t$src, $dst",
                         [(set i32:$dst, immSExt32:$src)]>;
}


// TODO check if DAG imm will be matched to i*imm operand
// (An) <- Imm
def MOV8mi  : M680x0Inst<0x1000, (outs), (ins M680x0_ARI8:$dst, i8imm:$src),
                         "move.b\t$src, $dst",
                         [(store immSExt8:$src, iPTR:$dst)]>;

def MOV16mi : M680x0Inst<0x3000, (outs), (ins M680x0_ARI16:$dst, i16imm:$src),
                         "move.w\t$src, $dst",
                         [(store immSExt16:$src, iPTR:$dst)]>;

def MOV32mi : M680x0Inst<0x2000, (outs), (ins M680x0_ARI32:$dst, i32imm:$src),
                         "move.l\t$src, $dst}",
                         [(store immSExt32:$src, iPTR:$dst)]>;


// Dn <- (An)
let canFoldAsLoad = 1, isReMaterializable = 1 in {
def MOV8rm  : M680x0Inst<0x1000, (outs DR8:$dst), (ins M680x0_ARI8 :$src),
                         "move.b\t$src, $dst",
                         [(set DR8:$dst, (M680x0_loadi8 iPTR:$src))]>;

def MOV16rm : M680x0Inst<0x1000, (outs DR16:$dst), (ins M680x0_ARI16:$src),
                         "move.w\t$src, $dst",
                         [(set DR16:$dst, (M680x0_loadi16 iPTR:$src))]>;

def MOV32rm : M680x0Inst<0x1000, (outs DR32:$dst), (ins M680x0_ARI32:$src),
                         "move.l\t$src, $dst",
                         [(set DR32:$dst, (M680x0_loadi32 iPTR:$src))]>;


// (An) <- Dn
def MOV8mr  : M680x0Inst<0x1000, (outs), (ins M680x0_ARI8:$dst, DR8:$src),
                         "move.b\t$src, $dst",
                         [(store DR8:$src, iPTR:$dst)]>;

def MOV16mr : M680x0Inst<0x1000, (outs), (ins M680x0_ARI16:$dst, DR16:$src),
                         "move.w\t$src, $dst",
                         [(store DR16:$src, iPTR:$dst)]>;

def MOV32mr : M680x0Inst<0x1000, (outs), (ins M680x0_ARI32:$dst, DR32:$src),
                         "move.l\t$src, $dst",
                         [(store DR32:$src, iPTR:$dst)]>;
}
} // Defs


// ARITHMETIC
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
def ADD8ri  : M680x0Inst<0x000,
               (outs M680x0_DRD8:$dst), (ins M680x0_DRD8:$src, i8imm:$imm),
               "addi.b\t$imm, $dst",
               [(set i8:$dst, (add i8:$src, immSExt8:$imm))]>;

def ADD16ri : M680x0Inst<0x000,
               (outs M680x0_DRD16:$dst), (ins M680x0_DRD16:$src, i16imm:$imm),
               "addi.w\t$imm, $dst",
               [(set i16:$dst, (add i16:$src, immSExt16:$imm))]>;

def ADD32ri : M680x0Inst<0x000,
               (outs M680x0_DRD32:$dst), (ins M680x0_DRD32:$src, i32imm:$imm),
               "addi.l\t$imm, $dst",
               [(set i32:$dst, (add i32:$src, immSExt32:$imm))]>;

// Dn <- Dn + Dr
def ADD8rr  : M680x0Inst<0x000,
               (outs M680x0_DRD8:$dst), (ins M680x0_DRD8:$src, M680x0_DRD8:$reg),
               "add.b\t$reg, $dst",
               [(set i8:$dst, (add i8:$src, i8:$reg))]>;

def ADD16rr : M680x0Inst<0x000,
               (outs M680x0_DRD16:$dst), (ins M680x0_DRD16:$src, M680x0_DRD16:$reg),
               "add.w\t$reg, $dst",
               [(set i16:$dst, (add i16:$src, i16:$reg))]>;

def ADD32rr : M680x0Inst<0x000,
               (outs M680x0_DRD32:$dst), (ins M680x0_DRD32:$src, M680x0_DRD32:$reg),
               "add.l\t$reg, $dst",
               [(set i32:$dst, (add i32:$src, i32:$reg))]>;


// Dn <- Dn - Imm
def SUB8ri  : M680x0Inst<0x000,
               (outs M680x0_DRD8:$dst), (ins M680x0_DRD8:$src, i8imm:$imm),
               "subi.b\t$imm, $dst",
               [(set i8:$dst, (sub i8:$src, immSExt8:$imm))]>;

def SUB16ri : M680x0Inst<0x000,
               (outs M680x0_DRD16:$dst), (ins M680x0_DRD16:$src, i16imm:$imm),
               "subi.w\t$imm, $dst",
               [(set i16:$dst, (sub i16:$src, immSExt16:$imm))]>;

def SUB32ri : M680x0Inst<0x000,
               (outs M680x0_DRD32:$dst), (ins M680x0_DRD32:$src, i32imm:$imm),
               "subi.l\t$imm, $dst",
               [(set i32:$dst, (sub i32:$src, immSExt32:$imm))]>;

// Dn <- Dn - Dr
def SUB8rr  : M680x0Inst<0x000,
               (outs M680x0_DRD8:$dst), (ins M680x0_DRD8:$src, M680x0_DRD8:$reg),
               "sub.b\t$reg, $dst",
               [(set i8:$dst, (sub i8:$src, i8:$reg))]>;

def SUB16rr : M680x0Inst<0x000,
               (outs M680x0_DRD16:$dst), (ins M680x0_DRD16:$src, M680x0_DRD16:$reg),
               "sub.w\t$reg, $dst",
               [(set i16:$dst, (sub i16:$src, i16:$reg))]>;

def SUB32rr : M680x0Inst<0x000,
               (outs M680x0_DRD32:$dst), (ins M680x0_DRD32:$src, M680x0_DRD32:$reg),
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
def AND8ri  : M680x0Inst<0x000,
               (outs M680x0_DRD8:$dst), (ins M680x0_DRD8:$src, i8imm:$imm),
               "andi.b\t$imm, $dst",
               [(set i8:$dst, (and i8:$src, immSExt8:$imm))]>;

def AND16ri : M680x0Inst<0x000,
               (outs M680x0_DRD16:$dst), (ins M680x0_DRD16:$src, i16imm:$imm),
               "andi.w\t$imm, $dst",
               [(set i16:$dst, (and i16:$src, immSExt16:$imm))]>;

def AND32ri : M680x0Inst<0x000,
               (outs M680x0_DRD32:$dst), (ins M680x0_DRD32:$src, i32imm:$imm),
               "andi.l\t$imm, $dst",
               [(set i32:$dst, (and i32:$src, immSExt32:$imm))]>;

// Dn <- Dn AND Dr
def AND8rr  : M680x0Inst<0x000,
               (outs M680x0_DRD8:$dst), (ins M680x0_DRD8:$src, M680x0_DRD8:$reg),
               "and.b\t$reg, $dst",
               [(set i8:$dst, (and i8:$src, i8:$reg))]>;

def AND16rr : M680x0Inst<0x000,
               (outs M680x0_DRD16:$dst), (ins M680x0_DRD16:$src, M680x0_DRD16:$reg),
               "and.w\t$reg, $dst",
               [(set i16:$dst, (and i16:$src, i16:$reg))]>;

def AND32rr : M680x0Inst<0x000,
               (outs M680x0_DRD32:$dst), (ins M680x0_DRD32:$src, M680x0_DRD32:$reg),
               "and.l\t$reg, $dst",
               [(set i32:$dst, (and i32:$src, i32:$reg))]>;
} // CCR Defs
} // Constraints
} // isCommutable


// CONTROL

// Indirect branching
let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
// FIXME M680x0 does not allow 16 bit indirect jumps use sext operands
/* def JMP16r     : M680x0Inst<0x0 (outs), (ins M680x0_ARI16:$dst), */
/*                             "jmp\t$dst", */
/*                             [(brind AR16:$dst)]>; */

// jmp (An)
def JMP32r     : M680x0Inst<0x0, (outs), (ins M680x0_ARI32:$dst),
                            "jmp\t$dst",
                            [(brind iPTR:$dst)]>;

// FIXME how to jmp from mem?
/* def JMP16m     : M680x0Inst<0x0 (outs), (ins M680x0_ARI16:$dst), */
/*                             "jmp{w}\t{*}$dst", */
/*                             [(brind (M680x0_loadi16 AR32:$dst))]>; */

/* def JMP32m     : M680x0Inst<0x0 (outs), (ins M680x0_ARI32:$dst), */
/*                             "jmp\t$dst", */
/*                             [(brind (M680x0_loadi32 AR32:$dst))]>; */
}

// TODO don't forget about LINK/UNLK
// JSR,LINK ... UNLK,RTS(RTR)
// Return
let isTerminator = 1, isReturn = 1, isBarrier = 1, hasCtrlDep = 1 in {
def RTS : M680x0Inst<0x0, (outs), (ins), "rts", []>;
let isCodeGenOnly = 1 in {
def RET : M680x0PseudoInst<(outs), (ins i32imm:$adj, variable_ops),
                           "", [(M680x0Ret timm:$adj)]>;
} // isCodeGenOnly
}
