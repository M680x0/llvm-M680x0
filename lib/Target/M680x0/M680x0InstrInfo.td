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

def MxSDT_CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>]>;
def MxSDT_CallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;

def MxSDT_Call    : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;

def MxSDT_Ret     : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;
def MxSDT_TCRet   : SDTypeProfile<0, 2, [SDTCisPtrTy<0>, SDTCisVT<1, i32>]>;

def MxSDT_Wrapper : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;

def MxSDT_UnArithCCROut : SDTypeProfile<2, 1, [
  /* RES */ SDTCisInt<0>,
  /* CCR */ SDTCisVT<1, i8>,
  /* OPD */ SDTCisSameAs<0, 2>
]>;

// RES, CCR <- op LHS, RHS
def MxSDT_BiArithCCROut : SDTypeProfile<2, 2, [
  /* RES */ SDTCisInt<0>,
  /* CCR */ SDTCisVT<1, i8>,
  /* LHS */ SDTCisSameAs<0, 2>,
  /* RHS */ SDTCisSameAs<0, 3>
]>;

// RES, CCR <- op LHS, RHS, CCR
def MxSDT_BiArithCCRInOut : SDTypeProfile<2, 3, [
  /* RES 1 */ SDTCisInt<0>,
  /*   CCR */ SDTCisVT<1, i8>,
  /*   LHS */ SDTCisSameAs<0, 2>,
  /*   RHS */ SDTCisSameAs<0, 3>,
  /*   CCR */ SDTCisSameAs<1, 4>
]>;

// RES1, RES2, CCR <- op LHS, RHS
def MxSDT_2BiArithCCROut : SDTypeProfile<3, 2, [
  /* RES 1 */ SDTCisInt<0>,
  /* RES 2 */ SDTCisSameAs<0, 1>,
  /*   CCR */ SDTCisVT<1, i8>,
  /*   LHS */ SDTCisSameAs<0, 2>,
  /*   RHS */ SDTCisSameAs<0, 3>
]>;

def MxSDT_CmpTest : SDTypeProfile<1, 2, [
   /* CCR */ SDTCisVT<0, i8>,
   /* Ops */ SDTCisSameAs<1, 2>
]>;

def MxSDT_BrCond : SDTypeProfile<0, 3, [
  /* Dest */ SDTCisVT<0, OtherVT>,
  /* Cond */ SDTCisVT<1, i8>,
  /*  CCR */ SDTCisVT<2, i8>
]>;

def MxSDT_SetCC : SDTypeProfile<1, 2, [
  /* BOOL */ SDTCisVT<0, i8>,
  /* Cond */ SDTCisVT<1, i8>,
  /*  CCR */ SDTCisVT<2, i8>
]>;

def MxSDT_SetCC_C : SDTypeProfile<1, 2, [
  /* BOOL */ SDTCisInt<0>,
  /* Cond */ SDTCisVT<1, i8>,
  /*  CCR */ SDTCisVT<2, i8>
]>;


//===----------------------------------------------------------------------===//
// Nodes
//===----------------------------------------------------------------------===//

def MxCallSeqStart : SDNode<"ISD::CALLSEQ_START", MxSDT_CallSeqStart,
                            [SDNPHasChain, SDNPOutGlue]>;

def MxCallSeqEnd   : SDNode<"ISD::CALLSEQ_END", MxSDT_CallSeqEnd,
                            [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def MxCall         : SDNode<"M680x0ISD::CALL", MxSDT_Call,
                            [SDNPHasChain, SDNPOutGlue,
                             SDNPOptInGlue, SDNPVariadic]>;

def MxRet   : SDNode<"M680x0ISD::RET", MxSDT_Ret,
                     [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def MxTCRet : SDNode<"M680x0ISD::TC_RETURN", MxSDT_TCRet,
                     [SDNPHasChain,  SDNPOptInGlue, SDNPVariadic]>;

def MxWrapper   : SDNode<"M680x0ISD::Wrapper",   MxSDT_Wrapper>;
def MxWrapperPC : SDNode<"M680x0ISD::WrapperPC", MxSDT_Wrapper>;

def MxAdd  : SDNode<"M680x0ISD::ADD",  MxSDT_BiArithCCROut, [SDNPCommutative]>;
def MxSub  : SDNode<"M680x0ISD::SUB",  MxSDT_BiArithCCROut>;
def MxOr   : SDNode<"M680x0ISD::OR",   MxSDT_BiArithCCROut, [SDNPCommutative]>;
def MxXor  : SDNode<"M680x0ISD::XOR",  MxSDT_BiArithCCROut, [SDNPCommutative]>;
def MxAnd  : SDNode<"M680x0ISD::AND",  MxSDT_BiArithCCROut, [SDNPCommutative]>;

def MxAddX : SDNode<"M680x0ISD::ADDX", MxSDT_BiArithCCRInOut>;
def MxSubX : SDNode<"M680x0ISD::SUBX", MxSDT_BiArithCCRInOut>;

def MxSMul : SDNode<"M680x0ISD::SMUL", MxSDT_BiArithCCROut, [SDNPCommutative]>;
def MxUMul : SDNode<"M680x0ISD::UMUL", MxSDT_2BiArithCCROut, [SDNPCommutative]>;

def MxCmp     : SDNode<"M680x0ISD::CMP", MxSDT_CmpTest>;
def MxBt      : SDNode<"M680x0ISD::BT",  MxSDT_CmpTest>;

def MxBrcond  : SDNode<"M680x0ISD::BRCOND", MxSDT_BrCond, [SDNPHasChain]>;
def MxSetcc   : SDNode<"M680x0ISD::SETCC",  MxSDT_SetCC>;
def MxSetcc_c : SDNode<"M680x0ISD::SETCC_CARRY", MxSDT_SetCC_C>;


//===----------------------------------------------------------------------===//
// Operands
//===----------------------------------------------------------------------===//

/// Size is the size of the data, either bits of a register or number of bits
/// addressed in memory. Size id is a letter that identifies size.
class MxSize<int num, string id, string full> {
  int Num = num;
  string Id = id;
  string Full = full;
}

def MxSize8  : MxSize<8,  "b", "byte">;
def MxSize16 : MxSize<16, "w", "word">;
def MxSize32 : MxSize<32, "l", "long">;

class MxOperand<ValueType vt, MxSize size, string letter, RegisterClass rc, dag pat = (null_frag)> {
  ValueType VT = vt;
  string Letter = letter;
  MxSize Size = size;
  RegisterClass RC = rc;
  dag Pat = pat;
}

class MxRegOp<ValueType vt,
              RegisterClass rc,
              MxSize size,
              string letter,
              string pm = "printOperand">
    : RegisterOperand<rc, pm>,
      MxOperand<vt, size, letter, rc>;

// REGISTER DIRECT. The operand is in the data register specified by
// the effective address register field.
def MxXRD16 : MxRegOp<i16, XR16, MxSize16, "r">;
def MxXRD32 : MxRegOp<i32, XR32, MxSize32, "r">;

def MxXRD16_TC : MxRegOp<i16, XR16_TC, MxSize16, "r">;
def MxXRD32_TC : MxRegOp<i32, XR32_TC, MxSize32, "r">;

// DATA REGISTER DIRECT. The operand is in the data register specified by
// the effective address register field.
def MxDRD8  : MxRegOp<i8,  DR8,  MxSize8,  "d">;
def MxDRD16 : MxRegOp<i16, DR16, MxSize16, "d">;
def MxDRD32 : MxRegOp<i32, DR32, MxSize32, "d">;

def MxDRD16_TC : MxRegOp<i16, DR16_TC, MxSize16, "d">;
def MxDRD32_TC : MxRegOp<i32, DR32_TC, MxSize32, "d">;

// ADDRESS REGISTER DIRECT. The operand is in the address register specified by
// the effective address register field.
def MxARD16 : MxRegOp<i16, AR16, MxSize16, "a">;
def MxARD32 : MxRegOp<i32, AR32, MxSize32, "a">;

def MxARD16_TC : MxRegOp<i16, AR16_TC, MxSize16, "a">;
def MxARD32_TC : MxRegOp<i32, AR32_TC, MxSize32, "a">;

// TODO finish parser wiring
def MxMemAsmOperand : AsmOperandClass {
 let Name = "MxMemOp";
}

class MxMemOp<dag ops, MxSize size, string letter,
              string printMethod = "printOperand",
              AsmOperandClass parserMatchClass = MxMemAsmOperand>
    : Operand<iPTR>, MxOperand<iPTR, size, letter, ?> {
  let PrintMethod = printMethod;
  let MIOperandInfo = ops;
  let ParserMatchClass = parserMatchClass;
  let OperandType = "OPERAND_MEMORY";
}

// ADDRESS REGISTER INDIRECT. The address of the operand is in the address
// register specified by the register field. The reference is classified as
// a data reference with the exception of the jump and jump-to-subroutine
// instructions.
def MxARI8        : MxMemOp<(ops AR32), MxSize8,  "j", "printARI8Mem">;
def MxARI16       : MxMemOp<(ops AR32), MxSize16, "j", "printARI16Mem">;
def MxARI32       : MxMemOp<(ops AR32), MxSize32, "j", "printARI32Mem">;

def MxARI8_TC     : MxMemOp<(ops AR32_TC), MxSize8,  "j", "printARI8Mem">;
def MxARI16_TC    : MxMemOp<(ops AR32_TC), MxSize16, "j", "printARI16Mem">;
def MxARI32_TC    : MxMemOp<(ops AR32_TC), MxSize32, "j", "printARI32Mem">;

// ADDRESS REGISTER INDIRECT WITH POSTINCREMENT. The address of the operand is
// in the address register specified by the register field. After the operand
// address is used, it is incremented by one, two, or four depending upon whether
// the size of the operand is byte, word, or long word. If the address register
// is the stack pointer and the operand size is byte, the address is incremented
// by two rather than one to keep the stack pointer on a word boundary.
// The reference is classified as a data reference.
def MxARIPI8      : MxMemOp<(ops AR32), MxSize8,  "o", "printARIPI8Mem">;
def MxARIPI16     : MxMemOp<(ops AR32), MxSize16, "o", "printARIPI16Mem">;
def MxARIPI32     : MxMemOp<(ops AR32), MxSize32, "o", "printARIPI32Mem">;

def MxARIPI8_TC   : MxMemOp<(ops AR32_TC), MxSize8,  "o", "printARIPI8Mem">;
def MxARIPI16_TC  : MxMemOp<(ops AR32_TC), MxSize16, "o", "printARIPI16Mem">;
def MxARIPI32_TC  : MxMemOp<(ops AR32_TC), MxSize32, "o", "printARIPI32Mem">;

// ADDRESS REGISTER INDIRECT WITH PREDECREMENT. The address of the operand is in
// the address register specified by the register field. Before the operand
// address is used, it is decremented by one, two, or four depending upon whether
// the operand size is byte, word, or long word. If the address register is
// the stack pointer and the operand size is byte, the address is decremented by
// two rather than one to keep the stack pointer on a word boundary.
// The reference is classified as a data reference.
def MxARIPD8      : MxMemOp<(ops AR32), MxSize8,  "e", "printARIPD8Mem">;
def MxARIPD16     : MxMemOp<(ops AR32), MxSize16, "e", "printARIPD16Mem">;
def MxARIPD32     : MxMemOp<(ops AR32), MxSize32, "e", "printARIPD32Mem">;

def MxARIPD8_TC   : MxMemOp<(ops AR32_TC), MxSize8,  "e", "printARIPD8Mem">;
def MxARIPD16_TC  : MxMemOp<(ops AR32_TC), MxSize16, "e", "printARIPD16Mem">;
def MxARIPD32_TC  : MxMemOp<(ops AR32_TC), MxSize32, "e", "printARIPD32Mem">;

// ADDRESS REGISTER INDIRECT WITH DISPLACEMENT. This addressing mode requires one
// word of extension. The address of the operand is the sum of the address in
// the address register and the sign-extended 16-bit displacement integer in the
// extension word. The reference is classified as a data reference with the
// exception of the jump and jump-to-subroutine instructions.
def MxARID8       : MxMemOp<(ops i16imm, AR32), MxSize8,  "p", "printARID8Mem">;
def MxARID16      : MxMemOp<(ops i16imm, AR32), MxSize16, "p", "printARID16Mem">;
def MxARID32      : MxMemOp<(ops i16imm, AR32), MxSize32, "p", "printARID32Mem">;

def MxARID8_TC    : MxMemOp<(ops i16imm, AR32_TC), MxSize8,  "p", "printARID8Mem">;
def MxARID16_TC   : MxMemOp<(ops i16imm, AR32_TC), MxSize16, "p", "printARID16Mem">;
def MxARID32_TC   : MxMemOp<(ops i16imm, AR32_TC), MxSize32, "p", "printARID32Mem">;

// ADDRESS REGISTER INDIRECT WITH INDEX. This addressing mode requires one word
// of extension. The address of the operand is the sum of the address in the
// address register, the signextended displacement integer in the low order eight
// bits of the extension word, and the contents of the index register.
// The reference is classified as a data reference with the exception of the
// jump and jump-to-subroutine instructions
def MxARII8      : MxMemOp<(ops i8imm, AR32, XR32), MxSize8,  "f", "printARII8Mem">;
def MxARII16     : MxMemOp<(ops i8imm, AR32, XR32), MxSize16, "f", "printARII16Mem">;
def MxARII32     : MxMemOp<(ops i8imm, AR32, XR32), MxSize32, "f", "printARII32Mem">;

def MxARII8_TC   : MxMemOp<(ops i8imm, AR32_TC, XR32_TC), MxSize8,  "f", "printARII8Mem">;
def MxARII16_TC  : MxMemOp<(ops i8imm, AR32_TC, XR32_TC), MxSize16, "f", "printARII16Mem">;
def MxARII32_TC  : MxMemOp<(ops i8imm, AR32_TC, XR32_TC), MxSize32, "f", "printARII32Mem">;

// ABSOLUTE SHORT ADDRESS. This addressing mode requires one word of extension.
// The address of the operand is the extension word. The 16-bit address is sign
// extended before it is used.  The reference is classified as a data reference
// with the exception of the jump and jump-tosubroutine instructions.
def MxAS8      : MxMemOp<(ops i16imm), MxSize8,  "b", "printAS8Mem">;
def MxAS16     : MxMemOp<(ops i16imm), MxSize16, "b", "printAS16Mem">;
def MxAS32     : MxMemOp<(ops i16imm), MxSize32, "b", "printAS32Mem">;

// ABSOLUTE LONG ADDRESS. This addressing mode requires two words of extension.
// The address of the operand is developed by the concatenation of the extension
// words. The high order part of the address is the first extension word; the low
// order part of the address is the second extension word. The reference is
// classified as a data reference with the exception of the jump and jump
// to-subroutine instructions.
def MxAL8      : MxMemOp<(ops i32imm), MxSize8,  "b", "printAL8Mem">;
def MxAL16     : MxMemOp<(ops i32imm), MxSize16, "b", "printAL16Mem">;
def MxAL32     : MxMemOp<(ops i32imm), MxSize32, "b", "printAL32Mem">;

let OperandType = "OPERAND_PCREL" in {
// PROGRAM COUNTER WITH DISPLACEMENT. This addressing mode requires one word of
// extension. The address of the operand is the sum of the address in the program
// counter and the Sign-extended 16-bit displacement integer in the extension
// word. The value in the program counter is the address of the extension word.
// The reference is classified as a program reference.
def MxPCD8     : MxMemOp<(ops i16imm), MxSize8,  "q", "printPCD8Mem">;
def MxPCD16    : MxMemOp<(ops i16imm), MxSize16, "q", "printPCD16Mem">;
def MxPCD32    : MxMemOp<(ops i16imm), MxSize32, "q", "printPCD32Mem">;

// PROGRAM COUNTER WITH INDEX. This addressing mode requires one word of
// extension. The address is the sum of the address in the program counter, the
// sign-extended displacement integer in the lower eight bits of the extension
// word, and the contents of the index register.  The value in the program
// counter is the address of the extension word. This reference is classified as
// a program reference.
def MxPCI8   : MxMemOp<(ops i8imm, XR32), MxSize8,  "k", "printPCI8Mem">;
def MxPCI16  : MxMemOp<(ops i8imm, XR32), MxSize16, "k", "printPCI16Mem">;
def MxPCI32  : MxMemOp<(ops i8imm, XR32), MxSize32, "k", "printPCI32Mem">;
} // OPERAND_PCREL

class MxOp<ValueType vt, MxSize size, string letter>
    : Operand<vt>,
      MxOperand<vt, size, letter, ?>;

let OperandType = "OPERAND_IMMEDIATE" in {
// IMMEDIATE DATA. This addressing mode requires either one or two words of
// extension depending on the size of the operation.
//     Byte Operation - operand is low order byte of extension word
//     Word Operation - operand is extension word
//     Long Word Operation - operand is in the two extension words,
//                           high order 16 bits are in the first
//                           extension word, low order 16 bits are
//                           in the second extension word.
def Mxi8imm  : MxOp<i8,  MxSize8,  "i">;
def Mxi16imm : MxOp<i16, MxSize16, "i">;
def Mxi32imm : MxOp<i32, MxSize32, "i">;
} // OPERAND_IMMEDIATE

let OperandType = "OPERAND_PCREL",
    /* ParserMatchClass = M680x0AbsMemAsmOperand, */
    PrintMethod = "printPCRelImm" in {

// Branch targets have OtherVT type and print as pc-relative values.
def MxBrTarget8  : Operand<OtherVT>;
def MxBrTarget16 : Operand<OtherVT>;
def MxBrTarget32 : Operand<OtherVT>;

} // OPERAND_PCREL


//===----------------------------------------------------------------------===//
// Predicates
//===----------------------------------------------------------------------===//

def SmallCode    : Predicate<"TM.getCodeModel() == CodeModel::Small">;
def KernelCode   : Predicate<"TM.getCodeModel() == CodeModel::Kernel">;
def FarData      : Predicate<"TM.getCodeModel() != CodeModel::Small &&"
                             "TM.getCodeModel() != CodeModel::Kernel">;
def NearData     : Predicate<"TM.getCodeModel() == CodeModel::Small ||"
                             "TM.getCodeModel() == CodeModel::Kernel">;
def IsPIC        : Predicate<"TM.isPositionIndependent()">;
def IsNotPIC     : Predicate<"!TM.isPositionIndependent()">;
def IsM68000     : Predicate<"!Subtarget.IsM68000()">;
def IsM68010     : Predicate<"!Subtarget.IsM68010()">;
def IsM68020     : Predicate<"!Subtarget.IsM68020()">;
def IsM68030     : Predicate<"!Subtarget.IsM68030()">;
def IsM68040     : Predicate<"!Subtarget.IsM68040()">;


//===----------------------------------------------------------------------===//
// Condition Codes
//
// These MUST be kept in sync with codes enum in M680x0InstrInfo.h
//===----------------------------------------------------------------------===//

def MxCOND_T   : PatLeaf<(i8 0)>;  // True
def MxCOND_F   : PatLeaf<(i8 1)>;  // False
def MxCOND_HI  : PatLeaf<(i8 2)>;  // High
def MxCOND_LS  : PatLeaf<(i8 3)>;  // Less or Same
def MxCOND_CC  : PatLeaf<(i8 4)>;  // Carry Clear
def MxCOND_CS  : PatLeaf<(i8 5)>;  // Carry Set
def MxCOND_NE  : PatLeaf<(i8 6)>;  // Not Equal
def MxCOND_EQ  : PatLeaf<(i8 7)>;  // Equal
def MxCOND_VC  : PatLeaf<(i8 8)>;  // Overflow Clear
def MxCOND_VS  : PatLeaf<(i8 9)>;  // Overflow Set
def MxCOND_PL  : PatLeaf<(i8 10)>; // Plus
def MxCOND_MI  : PatLeaf<(i8 11)>; // Minus
def MxCOND_GE  : PatLeaf<(i8 12)>; // Greater or Equal
def MxCOND_LT  : PatLeaf<(i8 13)>; // Less Than
def MxCOND_GT  : PatLeaf<(i8 14)>; // Greater Than
def MxCOND_LE  : PatLeaf<(i8 15)>; // Less or Equal


//===----------------------------------------------------------------------===//
// Complex Patterns
//===----------------------------------------------------------------------===//

// NOTE Though this CP is not stricly necessarily it will simplify instruciton
// definitions
def MxCP_ARI   : ComplexPattern<iPTR, 1, "SelectARI",
                                [], [SDNPWantParent]>;

def MxCP_ARIPI : ComplexPattern<iPTR, 1, "SelectARIPI",
                                [], [SDNPWantParent]>;

def MxCP_ARIPD : ComplexPattern<iPTR, 1, "SelectARIPD",
                                [], [SDNPWantParent]>;

def MxCP_ARID  : ComplexPattern<iPTR, 2, "SelectARID",
                                [add, sub, mul, or, shl, frameindex], [SDNPWantParent]>;

def MxCP_ARII  : ComplexPattern<iPTR, 3, "SelectARII",
                                [add, sub, mul, or, shl, frameindex],
                                [SDNPWantParent]>;

def MxCP_PCD   : ComplexPattern<iPTR, 1, "SelectPCD",
                                [], [SDNPWantParent]>;

def MxCP_PCI   : ComplexPattern<iPTR, 2, "SelectPCI",
                                [], [SDNPWantParent]>;


//===----------------------------------------------------------------------===//
// Pattern Fragments
//===----------------------------------------------------------------------===//

def MximmSExt8  : PatLeaf<(i8  imm)>;
def MximmSExt16 : PatLeaf<(i16 imm)>;
def MximmSExt32 : PatLeaf<(i32 imm)>;

// Used for Shifts and Rotations, since M680x0 immediates in these
// instructions are 1 <= i <= 8.
// Generally, if immediate is bigger than 8 it will be moved to a register
// and then an operation is performed. TODO you need to evaluate whether
// splitting one big shift(or rotate) into a few smaller is faster than doing
// a move, if so do custom lowering
def Mximm1to8   : ImmLeaf<i32, [{ return Imm >= 1 && Imm <= 8; }]>;

// Helper fragments for loads.
// It's always safe to treat a anyext i16 load as a i32 load if the i16 is
// known to be 32-bit aligned or better. Ditto for i8 to i16.
def Mxloadi16 : PatFrag<(ops node:$ptr), (i16 (unindexedload node:$ptr)), [{
  LoadSDNode *LD = cast<LoadSDNode>(N);
  ISD::LoadExtType ExtType = LD->getExtensionType();
  if (ExtType == ISD::NON_EXTLOAD)
    return true;
  if (ExtType == ISD::EXTLOAD)
    return LD->getAlignment() >= 2 && !LD->isVolatile();
  return false;
}]>;

def Mxloadi32 : PatFrag<(ops node:$ptr), (i32 (unindexedload node:$ptr)), [{
  LoadSDNode *LD = cast<LoadSDNode>(N);
  ISD::LoadExtType ExtType = LD->getExtensionType();
  if (ExtType == ISD::NON_EXTLOAD)
    return true;
  if (ExtType == ISD::EXTLOAD)
    return LD->getAlignment() >= 4 && !LD->isVolatile();
  return false;
}]>;

def Mxloadi8         : PatFrag<(ops node:$ptr), (i8  (load node:$ptr))>;

def MxSExtLoadi16i8  : PatFrag<(ops node:$ptr), (i16 (sextloadi8 node:$ptr))>;
def MxSExtLoadi32i8  : PatFrag<(ops node:$ptr), (i32 (sextloadi8 node:$ptr))>;
def MxSExtLoadi32i16 : PatFrag<(ops node:$ptr), (i32 (sextloadi16 node:$ptr))>;

def MxZExtLoadi8i1   : PatFrag<(ops node:$ptr), (i8  (zextloadi1 node:$ptr))>;
def MxZExtLoadi16i1  : PatFrag<(ops node:$ptr), (i16 (zextloadi1 node:$ptr))>;
def MxZExtLoadi32i1  : PatFrag<(ops node:$ptr), (i32 (zextloadi1 node:$ptr))>;
def MxZExtLoadi16i8  : PatFrag<(ops node:$ptr), (i16 (zextloadi8 node:$ptr))>;
def MxZExtLoadi32i8  : PatFrag<(ops node:$ptr), (i32 (zextloadi8 node:$ptr))>;
def MxZExtLoadi32i16 : PatFrag<(ops node:$ptr), (i32 (zextloadi16 node:$ptr))>;

def MxExtLoadi8i1    : PatFrag<(ops node:$ptr), (i8  (extloadi1 node:$ptr))>;
def MxExtLoadi16i1   : PatFrag<(ops node:$ptr), (i16 (extloadi1 node:$ptr))>;
def MxExtLoadi32i1   : PatFrag<(ops node:$ptr), (i32 (extloadi1 node:$ptr))>;
def MxExtLoadi16i8   : PatFrag<(ops node:$ptr), (i16 (extloadi8 node:$ptr))>;
def MxExtLoadi32i8   : PatFrag<(ops node:$ptr), (i32 (extloadi8 node:$ptr))>;
def MxExtLoadi32i16  : PatFrag<(ops node:$ptr), (i32 (extloadi16 node:$ptr))>;


//===----------------------------------------------------------------------===//
// Type Fixtures
//
// Type Fixtures are ValueType related information sets that usually go together
//===----------------------------------------------------------------------===//

// TODO make it folded like MxType8.F.Op nad MxType8.F.Pat
// TODO move strings into META subclass
class MxType<ValueType vt,  /// Type of data this fixture refers to
            string prefix,  /// Prefix used to identify type
           string postfix,  /// Prefix used to qualify type
              string rLet,  /// Register letter
            MxOperand rOp,  /// Supported any register operand
            MxOperand jOp,  /// Supported ARI operand
      ComplexPattern jPat,  /// What ARI pattern to use
            MxOperand oOp,  /// Supported ARIPI operand
      ComplexPattern oPat,  /// What ARIPI pattern is used
            MxOperand eOp,  /// Supported ARIPD operand
      ComplexPattern ePat,  /// What ARIPD pattern is used
            MxOperand pOp,  /// Supported ARID operand
      ComplexPattern pPat,  /// What ARID pattern is used
            MxOperand fOp,  /// Supported ARII operand
      ComplexPattern fPat,  /// What ARII pattern is used
            MxOperand iOp,  /// Supported immediate operand
             PatFrag iPat,  /// What immediate pattern is used
             PatFrag load   /// What load operation is used with MEM
             > {
  int Size = vt.Size;
  ValueType VT = vt;
  string Prefix = prefix;
  string Postfix = postfix;
  string RLet = rLet;
  MxOperand ROp = rOp;
  MxOperand JOp = jOp;
  ComplexPattern JPat = jPat;
  MxOperand OOp = oOp;
  ComplexPattern OPat = oPat;
  MxOperand EOp = eOp;
  ComplexPattern EPat = ePat;
  MxOperand POp = pOp;
  ComplexPattern PPat = pPat;
  MxOperand FOp = fOp;
  ComplexPattern FPat = fPat;
  MxOperand IOp = iOp;
  PatFrag IPat = iPat;
  PatFrag Load = load;
}

class MxType8Class<string rLet, MxOperand reg>
    : MxType<i8, "b", "", rLet, reg,
             MxARI8,   MxCP_ARI,
             MxARIPI8, MxCP_ARIPI,
             MxARIPD8, MxCP_ARIPD,
             MxARID8,  MxCP_ARID,
             MxARII8,  MxCP_ARII,
             Mxi8imm,  MximmSExt8,
             Mxloadi8>;

def MxType8 : MxType8Class<?,?>;

class MxType16Class<string rLet, MxOperand reg>
    : MxType<i16, "w", "", rLet, reg,
             MxARI16,   MxCP_ARI,
             MxARIPI16, MxCP_ARIPI,
             MxARIPD16, MxCP_ARIPD,
             MxARID16,  MxCP_ARID,
             MxARII16,  MxCP_ARII,
             Mxi16imm,  MximmSExt16,
             Mxloadi16>;

def MxType16 : MxType16Class<?,?>;

class MxType32Class<string rLet, MxOperand reg>
    : MxType<i32, "l", "", rLet, reg,
             MxARI32,   MxCP_ARI,
             MxARIPI32, MxCP_ARIPI,
             MxARIPD32, MxCP_ARIPD,
             MxARID32,  MxCP_ARID,
             MxARII32,  MxCP_ARII,
             Mxi32imm,  MximmSExt32,
             Mxloadi32>;

def MxType32 : MxType32Class<?,?>;


def MxType8d : MxType8Class<"d", MxDRD8>;

def MxType16d : MxType16Class<"d", MxDRD16>;
def MxType16a : MxType16Class<"a", MxARD16>;
def MxType16r : MxType16Class<"r", MxXRD16>;
def MxType32d : MxType32Class<"d", MxDRD32>;
def MxType32a : MxType32Class<"a", MxARD32>;
def MxType32r : MxType32Class<"r", MxXRD32>;

let Postfix = "_TC" in {
def MxType16d_TC : MxType16Class<"d", MxDRD16_TC>;
def MxType16a_TC : MxType16Class<"a", MxARD16_TC>;
def MxType16r_TC : MxType16Class<"r", MxXRD16_TC>;
def MxType32d_TC : MxType32Class<"d", MxDRD32_TC>;
def MxType32a_TC : MxType32Class<"a", MxARD32_TC>;
def MxType32r_TC : MxType32Class<"r", MxXRD32_TC>;
}


//===----------------------------------------------------------------------===//
// Subsystems
//===----------------------------------------------------------------------===//

include "M680x0InstrData.td"
include "M680x0InstrShiftRotate.td"
include "M680x0InstrArithmetic.td"
include "M680x0InstrControl.td"

include "M680x0InstrCompiler.td"
