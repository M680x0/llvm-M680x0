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

def MxSDT_Ret   : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;

def MxSDT_TCRet : SDTypeProfile<0, 2, [SDTCisPtrTy<0>, SDTCisVT<1, i32>]>;

//===----------------------------------------------------------------------===//
// Nodes
//===----------------------------------------------------------------------===//

def MxRet   : SDNode<"M680x0ISD::RET", MxSDT_Ret,
                     [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def MxTCRet : SDNode<"M680x0ISD::TC_RETURN", MxSDT_TCRet,
                     [SDNPHasChain,  SDNPOptInGlue, SDNPVariadic]>;

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

def Mx8  : MxSize<8,  "b", "byte">;
def Mx16 : MxSize<16, "w", "word">;
def Mx32 : MxSize<32, "l", "long">;

class MxOperand<MxSize size, string letter> {
  MxSize Size = size;
  string Letter = letter;
}

class MxRegOp<RegisterClass rc,
              MxSize size,
              string letter,
              string pm = "printOperand">
    : RegisterOperand<rc, pm>, MxOperand<size, letter>;

// REGISTER DIRECT. The operand is in the data register specified by
// the effective address register field.
def MxXRD16 : MxRegOp<XR16, Mx16, "r", "printOperand">;
def MxXRD32 : MxRegOp<XR32, Mx32, "r", "printOperand">;

def MxXRD16_TC : MxRegOp<XR16_TC, Mx16, "r", "printOperand">;
def MxXRD32_TC : MxRegOp<XR32_TC, Mx32, "r", "printOperand">;

// DATA REGISTER DIRECT. The operand is in the data register specified by
// the effective address register field.
def MxDRD8  : MxRegOp<DR8,  Mx8,  "d", "printOperand">;
def MxDRD16 : MxRegOp<DR16, Mx16, "d", "printOperand">;
def MxDRD32 : MxRegOp<DR32, Mx32, "d", "printOperand">;

def MxDRD16_TC : MxRegOp<DR16_TC, Mx16, "d", "printOperand">;
def MxDRD32_TC : MxRegOp<DR32_TC, Mx32, "d", "printOperand">;

// ADDRESS REGISTER DIRECT. The operand is in the address register specified by
// the effective address register field.
def MxARD16 : MxRegOp<AR16, Mx16, "a", "printARDOperand">;
def MxARD32 : MxRegOp<AR32, Mx32, "a", "printARDOperand">;

def MxARD16_TC : MxRegOp<AR16_TC, Mx16, "a", "printARDOperand">;
def MxARD32_TC : MxRegOp<AR32_TC, Mx32, "a", "printARDOperand">;

// TODO finish parser wiring
def MxMemAsmOperand : AsmOperandClass {
 let Name = "MxMemOp";
}

class MxMemOp<dag ops, MxSize size, string letter,
              string printMethod = "printOperand",
              AsmOperandClass parserMatchClass = MxMemAsmOperand>
    : Operand<iPTR>, MxOperand<size, letter> {
  let PrintMethod = printMethod;
  let MIOperandInfo = ops;
  let ParserMatchClass = parserMatchClass;
  let OperandType = "OPERAND_MEMORY";
}

// ADDRESS REGISTER INDIRECT. The address of the operand is in the address
// register specified by the register field. The reference is classified as
// a data reference with the exception of the jump and jump-to-subroutine
// instructions.
def MxARI8        : MxMemOp<(ops AR32), Mx8,  "j", "printARI8Mem">;
def MxARI16       : MxMemOp<(ops AR32), Mx16, "j", "printARI16Mem">;
def MxARI32       : MxMemOp<(ops AR32), Mx32, "j", "printARI32Mem">;

def MxARI8_TC     : MxMemOp<(ops AR32_TC), Mx8,  "j", "printARI8Mem">;
def MxARI16_TC    : MxMemOp<(ops AR32_TC), Mx16, "j", "printARI16Mem">;
def MxARI32_TC    : MxMemOp<(ops AR32_TC), Mx32, "j", "printARI32Mem">;

// ADDRESS REGISTER INDIRECT WITH POSTINCREMENT. The address of the operand is
// in the address register specified by the register field. After the operand
// address is used, it is incremented by one, two, or four depending upon whether
// the size of the operand is byte, word, or long word. If the address register
// is the stack pointer and the operand size is byte, the address is incremented
// by two rather than one to keep the stack pointer on a word boundary.
// The reference is classified as a data reference.
def MxARIPI8      : MxMemOp<(ops AR32), Mx8,  "o", "printARIPI8Mem">;
def MxARIPI16     : MxMemOp<(ops AR32), Mx16, "o", "printARIPI16Mem">;
def MxARIPI32     : MxMemOp<(ops AR32), Mx32, "o", "printARIPI32Mem">;

def MxARIPI8_TC   : MxMemOp<(ops AR32_TC), Mx8,  "o", "printARIPI8Mem">;
def MxARIPI16_TC  : MxMemOp<(ops AR32_TC), Mx16, "o", "printARIPI16Mem">;
def MxARIPI32_TC  : MxMemOp<(ops AR32_TC), Mx32, "o", "printARIPI32Mem">;

// ADDRESS REGISTER INDIRECT WITH PREDECREMENT. The address of the operand is in
// the address register specified by the register field. Before the operand
// address is used, it is decremented by one, two, or four depending upon whether
// the operand size is byte, word, or long word. If the address register is
// the stack pointer and the operand size is byte, the address is decremented by
// two rather than one to keep the stack pointer on a word boundary.
// The reference is classified as a data reference.
def MxARIPD8      : MxMemOp<(ops AR32), Mx8,  "e", "printARIPD8Mem">;
def MxARIPD16     : MxMemOp<(ops AR32), Mx16, "e", "printARIPD16Mem">;
def MxARIPD32     : MxMemOp<(ops AR32), Mx32, "e", "printARIPD32Mem">;

def MxARIPD8_TC   : MxMemOp<(ops AR32_TC), Mx8,  "e", "printARIPD8Mem">;
def MxARIPD16_TC  : MxMemOp<(ops AR32_TC), Mx16, "e", "printARIPD16Mem">;
def MxARIPD32_TC  : MxMemOp<(ops AR32_TC), Mx32, "e", "printARIPD32Mem">;

// ADDRESS REGISTER INDIRECT WITH DISPLACEMENT. This addressing mode requires one
// word of extension. The address of the operand is the sum of the address in
// the address register and the sign-extended 16-bit displacement integer in the
// extension word. The reference is classified as a data reference with the
// exception of the jump and jump-to-subroutine instructions.
def MxARID8       : MxMemOp<(ops i16imm, AR32), Mx8,  "p", "printARID8Mem">;
def MxARID16      : MxMemOp<(ops i16imm, AR32), Mx16, "p", "printARID16Mem">;
def MxARID32      : MxMemOp<(ops i16imm, AR32), Mx32, "p", "printARID32Mem">;

def MxARID8_TC    : MxMemOp<(ops i16imm, AR32_TC), Mx8,  "p", "printARID8Mem">;
def MxARID16_TC   : MxMemOp<(ops i16imm, AR32_TC), Mx16, "p", "printARID16Mem">;
def MxARID32_TC   : MxMemOp<(ops i16imm, AR32_TC), Mx32, "p", "printARID32Mem">;

// ADDRESS REGISTER INDIRECT WITH INDEX. This addressing mode requires one word
// of extension. The address of the operand is the sum of the address in the
// address register, the signextended displacement integer in the low order eight
// bits of the extension word, and the contents of the index register.
// The reference is classified as a data reference with the exception of the
// jump and jump-to-subroutine instructions
def MxARII8      : MxMemOp<(ops i8imm, AR32, XR32), Mx8,  "f", "printARII8Mem">;
def MxARII16     : MxMemOp<(ops i8imm, AR32, XR32), Mx16, "f", "printARII16Mem">;
def MxARII32     : MxMemOp<(ops i8imm, AR32, XR32), Mx32, "f", "printARII32Mem">;

def MxARII8_TC   : MxMemOp<(ops i8imm, AR32_TC, XR32_TC), Mx8,  "f", "printARII8Mem">;
def MxARII16_TC  : MxMemOp<(ops i8imm, AR32_TC, XR32_TC), Mx16, "f", "printARII16Mem">;
def MxARII32_TC  : MxMemOp<(ops i8imm, AR32_TC, XR32_TC), Mx32, "f", "printARII32Mem">;

// ABSOLUTE SHORT ADDRESS. This addressing mode requires one word of extension.
// The address of the operand is the extension word. The 16-bit address is sign
// extended before it is used.  The reference is classified as a data reference
// with the exception of the jump and jump-tosubroutine instructions.
def MxAS8      : MxMemOp<(ops i16imm), Mx8,  "b", "printAS8Mem">;
def MxAS16     : MxMemOp<(ops i16imm), Mx16, "b", "printAS16Mem">;
def MxAS32     : MxMemOp<(ops i16imm), Mx32, "b", "printAS32Mem">;

// ABSOLUTE LONG ADDRESS. This addressing mode requires two words of extension.
// The address of the operand is developed by the concatenation of the extension
// words. The high order part of the address is the first extension word; the low
// order part of the address is the second extension word. The reference is
// classified as a data reference with the exception of the jump and jump
// to-subroutine instructions.
def MxAL8      : MxMemOp<(ops i32imm), Mx8,  "b", "printAL8Mem">;
def MxAL16     : MxMemOp<(ops i32imm), Mx16, "b", "printAL16Mem">;
def MxAL32     : MxMemOp<(ops i32imm), Mx32, "b", "printAL32Mem">;

let OperandType = "OPERAND_PCREL" in {
// PROGRAM COUNTER WITH DISPLACEMENT. This addressing mode requires one word of
// extension. The address of the operand is the sum of the address in the program
// counter and the Sign-extended 16-bit displacement integer in the extension
// word. The value in the program counter is the address of the extension word.
// The reference is classified as a program reference.
def MxPCD8     : MxMemOp<(ops i16imm), Mx8,  "q", "printPCD8Mem">;
def MxPCD16    : MxMemOp<(ops i16imm), Mx16, "q", "printPCD16Mem">;
def MxPCD32    : MxMemOp<(ops i16imm), Mx32, "q", "printPCD32Mem">;

// PROGRAM COUNTER WITH INDEX. This addressing mode requires one word of
// extension. The address is the sum of the address in the program counter, the
// sign-extended displacement integer in the lower eight bits of the extension
// word, and the contents of the index register.  The value in the program
// counter is the address of the extension word. This reference is classified as
// a program reference.
def MxPCI8   : MxMemOp<(ops i8imm, XR32), Mx8,  "k", "printPCI8Mem">;
def MxPCI16  : MxMemOp<(ops i8imm, XR32), Mx16, "k", "printPCI16Mem">;
def MxPCI32  : MxMemOp<(ops i8imm, XR32), Mx32, "k", "printPCI32Mem">;
} // OPERAND_PCREL

class MxOp<ValueType vt, MxSize size, string letter>
    : Operand<vt>, MxOperand<size, letter>;

let OperandType = "OPERAND_IMMEDIATE" in {
// IMMEDIATE DATA. This addressing mode requires either one or two words of
// extension depending on the size of the operation.
//     Byte Operation - operand is low order byte of extension word
//     Word Operation - operand is extension word
//     Long Word Operation - operand is in the two extension words,
//                           high order 16 bits are in the first
//                           extension word, low order 16 bits are
//                           in the second extension word.
def Mxi8imm  : MxOp<i8,  Mx8,  "i">;
def Mxi16imm : MxOp<i16, Mx16, "i">;
def Mxi32imm : MxOp<i32, Mx32, "i">;
} // OPERAND_IMMEDIATE

let OperandType = "OPERAND_PCREL",
    /* ParserMatchClass = M680x0AbsMemAsmOperand, */
    PrintMethod = "printPCRelImm" in {

// TODO do i need this?
// Branch targets have OtherVT type and print as pc-relative values.
def MxBrTarget  : Operand<OtherVT>;
def MxBrTarget8 : Operand<OtherVT>;

} // OPERAND_PCREL

//===----------------------------------------------------------------------===//
// Complex Patterns
//===----------------------------------------------------------------------===//

def MxCP_ARIPI : ComplexPattern<iPTR, 1, "selectARIPI",
                                [], [SDNPWantParent]>;

def MxCP_ARIPD : ComplexPattern<iPTR, 1, "selectARIPD",
                                [], [SDNPWantParent]>;

def MxCP_ARID  : ComplexPattern<iPTR, 2, "selectARID",
                                [frameindex], [SDNPWantParent]>;

def MxCP_ARII  : ComplexPattern<iPTR, 3, "selectARII",
                                [frameindex], [SDNPWantParent]>;

def MxCP_PCD   : ComplexPattern<iPTR, 1, "selectPCD",
                                [], [SDNPWantParent]>;

def MxCP_PCI   : ComplexPattern<iPTR, 2, "selectPCI",
                                [], [SDNPWantParent]>;

//===----------------------------------------------------------------------===//
// Predicates
//===----------------------------------------------------------------------===//

def SmallCode    : Predicate<"TM.getCodeModel() == CodeModel::Small">;
def KernelCode   : Predicate<"TM.getCodeModel() == CodeModel::Kernel">;
def FarData      : Predicate<"TM.getCodeModel() != CodeModel::Small &&"
                             "TM.getCodeModel() != CodeModel::Kernel">;
def NearData     : Predicate<"TM.getCodeModel() == CodeModel::Small ||"
                             "TM.getCodeModel() == CodeModel::Kernel">;
def IsNotPIC     : Predicate<"!TM.isPositionIndependent()">;

//===----------------------------------------------------------------------===//
// Pattern Fragments
//===----------------------------------------------------------------------===//

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

def Mxsextloadi16i8  : PatFrag<(ops node:$ptr), (i16 (sextloadi8 node:$ptr))>;
def Mxsextloadi32i8  : PatFrag<(ops node:$ptr), (i32 (sextloadi8 node:$ptr))>;
def Mxsextloadi32i16 : PatFrag<(ops node:$ptr), (i32 (sextloadi16 node:$ptr))>;

def Mxzextloadi8i1   : PatFrag<(ops node:$ptr), (i8  (zextloadi1 node:$ptr))>;
def Mxzextloadi16i1  : PatFrag<(ops node:$ptr), (i16 (zextloadi1 node:$ptr))>;
def Mxzextloadi32i1  : PatFrag<(ops node:$ptr), (i32 (zextloadi1 node:$ptr))>;
def Mxzextloadi16i8  : PatFrag<(ops node:$ptr), (i16 (zextloadi8 node:$ptr))>;
def Mxzextloadi32i8  : PatFrag<(ops node:$ptr), (i32 (zextloadi8 node:$ptr))>;
def Mxzextloadi32i16 : PatFrag<(ops node:$ptr), (i32 (zextloadi16 node:$ptr))>;

def Mxextloadi8i1    : PatFrag<(ops node:$ptr), (i8  (extloadi1 node:$ptr))>;
def Mxextloadi16i1   : PatFrag<(ops node:$ptr), (i16 (extloadi1 node:$ptr))>;
def Mxextloadi32i1   : PatFrag<(ops node:$ptr), (i32 (extloadi1 node:$ptr))>;
def Mxextloadi16i8   : PatFrag<(ops node:$ptr), (i16 (extloadi8 node:$ptr))>;
def Mxextloadi32i8   : PatFrag<(ops node:$ptr), (i32 (extloadi8 node:$ptr))>;
def Mxextloadi32i16  : PatFrag<(ops node:$ptr), (i32 (extloadi16 node:$ptr))>;

def MximmSExt8  : ImmLeaf<i8,  [{ return isInt<8>(Imm);  }]>;
def MximmSExt16 : ImmLeaf<i16, [{ return isInt<16>(Imm); }]>;
def MximmSExt32 : ImmLeaf<i32, [{ return isInt<32>(Imm); }]>;

//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

// NOP
let hasSideEffects = 0 in {
  def NOP : MxInst<0x4E71, (outs), (ins), "nop", []>;
}


// TODO make move set imlicit CCR like M680x0 does
// Do i need this?

// MOVE
// X - not affected
// N,Z - set by operation
// V,C - cleared
let Defs = [CCR] in {
multiclass MxMOV8Inst<MxOperand OPD, dag PAT> {
// Dn <- OPD
def "MOV8d"#OPD.Letter
  : MxInst<0x0, (outs MxDRD8:$dst), (ins OPD:$src), "move.b\t$src, $dst",
           [(set i8:$dst, PAT)]>;

// (An) <- OPD
def "MOV8j"#OPD.Letter
  : MxInst<0x0, (outs), (ins MxARI8:$dst, OPD:$src), "move.b\t$src, $dst",
           [(store PAT, iPTR:$dst)]>;

// (i,An) <- OPD
def "MOV8p"#OPD.Letter
  : MxInst<0x0, (outs), (ins MxARID8:$dst, OPD:$src), "move.b\t$src, $dst",
           [(store PAT, MxCP_ARID:$dst)]>;
}

multiclass MxMOVInst<MxOperand OPD, dag PAT> {

// Set CS registers

let mayLoad = 1 in  {
def "MOV"#OPD.Size.Num#"d"#OPD.Letter
  : MxInst<0x0, (outs !cast<DAGOperand>("MxDRD"#OPD.Size.Num):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

def "MOV"#OPD.Size.Num#"a"#OPD.Letter
  : MxInst<0x0, (outs !cast<DAGOperand>("MxARD"#OPD.Size.Num):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

def "MOV"#OPD.Size.Num#"r"#OPD.Letter
  : MxInst<0x0, (outs !cast<DAGOperand>("MxXRD"#OPD.Size.Num):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;


// Set TC registers

def "MOV"#OPD.Size.Num#"d"#OPD.Letter#"_TC"
  : MxInst<0x0, (outs !cast<DAGOperand>("MxDRD"#OPD.Size.Num#"_TC"):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

def "MOV"#OPD.Size.Num#"a"#OPD.Letter#"_TC"
  : MxInst<0x0, (outs !cast<DAGOperand>("MxARD"#OPD.Size.Num#"_TC"):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

def "MOV"#OPD.Size.Num#"r"#OPD.Letter#"_TC"
  : MxInst<0x0, (outs !cast<DAGOperand>("MxXRD"#OPD.Size.Num#"_TC"):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

} // mayLoad = 1


// Store to MEM

// (An) <- OPD
def "MOV"#OPD.Size.Num#"j"#OPD.Letter
  : MxInst<0x0, (outs), (ins !cast<DAGOperand>("MxARI"#OPD.Size.Num):$dst, OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(store PAT, iPTR:$dst)]>;

// (i,An) <- OPD
def "MOV"#OPD.Size.Num#"p"#OPD.Letter
  : MxInst<0x0, (outs), (ins !cast<DAGOperand>("MxARID"#OPD.Size.Num):$dst, OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(store PAT, MxCP_ARID:$dst)]>;

// (i,An) <- OPD
def "MOV"#OPD.Size.Num#"f"#OPD.Letter
  : MxInst<0x0, (outs), (ins !cast<DAGOperand>("MxARII"#OPD.Size.Num):$dst, OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(store PAT, MxCP_ARII:$dst)]>;
}
} // Defs = [CCR]

defm MOV8_d  : MxMOV8Inst<MxDRD8, (i8  imm:$src)>;
defm MOV16_d : MxMOVInst<MxDRD16, (i16 imm:$src)>;
defm MOV32_d : MxMOVInst<MxDRD32, (i32 imm:$src)>;

defm MOV16_a : MxMOVInst<MxARD16, (i16 imm:$src)>;
defm MOV32_a : MxMOVInst<MxARD32, (i32 imm:$src)>;

defm MOV16_r : MxMOVInst<MxXRD16, (i16 imm:$src)>;
defm MOV32_r : MxMOVInst<MxXRD32, (i32 imm:$src)>;

defm MOV8_i  : MxMOV8Inst<Mxi8imm, (i8 MximmSExt8:$src)>;
defm MOV16_i : MxMOVInst<Mxi16imm, (i16 MximmSExt16:$src)>;
defm MOV32_i : MxMOVInst<Mxi32imm, (i32 MximmSExt32:$src)>;

defm MOV8_j  : MxMOV8Inst<MxARI8, (Mxloadi8  iPTR:$src)>;
defm MOV16_j : MxMOVInst<MxARI16, (Mxloadi16 iPTR:$src)>;
defm MOV32_j : MxMOVInst<MxARI32, (Mxloadi32 iPTR:$src)>;

defm MOV8_p  : MxMOV8Inst<MxARID8, (i8 MxCP_ARID:$src)>;
defm MOV16_p : MxMOVInst<MxARID16, (i16 MxCP_ARID:$src)>;
defm MOV32_p : MxMOVInst<MxARID32, (i32 MxCP_ARID:$src)>;

defm MOV8_f  : MxMOV8Inst<MxARII8, (i8 MxCP_ARII:$src)>;
defm MOV16_f : MxMOVInst<MxARII16, (i16 MxCP_ARII:$src)>;
defm MOV32_f : MxMOVInst<MxARII32, (i32 MxCP_ARII:$src)>;


// FIXME Could these be specialized from normal MOVrPIm and MOVrpDm?
let Defs = [SP], hasSideEffects = 0 in {

// SP <- SP - <size>; (SP) <- Dn
let mayStore = 1 in {
def PUSH8r  : MxInst<0x0, (outs), (ins DR8:$reg,  CSP:$sp),
                         "move.b\t$reg, -($sp)",[]>;
def PUSH16r : MxInst<0x0, (outs), (ins DR16:$reg, CSP:$sp),
                         "move.w\t$reg, -($sp)",[]>;
def PUSH32r : MxInst<0x0, (outs), (ins DR32:$reg, CSP:$sp),
                         "move.l\t$reg, -($sp)",[]>;
} // let mayStore = 1

// Dn <- (SP); SP <- SP + <size>
let mayLoad = 1 in {
def POP8r  : MxInst<0x0, (outs DR8:$reg),  (ins CSP:$sp),
                        "move.b\t($sp)+, $reg", []>;
def POP16r : MxInst<0x0, (outs DR16:$reg), (ins CSP:$sp),
                        "move.w\t($sp)+, $reg", []>;
def POP32r : MxInst<0x0, (outs DR32:$reg), (ins CSP:$sp),
                        "move.l\t($sp)+, $reg", []>;
} // let mayLoad = 1

} // let Defs = [SP], hasSideEffects = 0


// LEA - Load Effective Address
def LEA32r_ARID : MxInst<0x0, (outs AR32:$dst), (ins MxARID32:$src),
                        "lea\t$src, $dst",
                        [(set i32:$dst, MxCP_ARID:$src)]>;

def LEA32r_ARII : MxInst<0x0, (outs AR32:$dst), (ins MxARII32:$src),
                        "lea\t$src, $dst",
                        [(set i32:$dst, MxCP_ARII:$src)]>;


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


//===----------------------------------------------------------------------===//
// Subsystems.
//===----------------------------------------------------------------------===//

include "M680x0InstrControl.td"

// Compiler Pseudo Instructions and Pat Patterns
include "M680x0InstrCompiler.td"
