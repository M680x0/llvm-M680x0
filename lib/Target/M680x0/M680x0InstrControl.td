//===-- M680x0InstrControl.td - Control Flow Instructions --*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the M680x0 jump, return, call, and related instructions.
// Here is the current status of the file:
//
//  Machine:
//
//       BRA   [x]     BSR  [ ]     Bcc [ ]     DBcc [ ]     FBcc [ ]
//       FDBcc [ ]     FNOP [ ]     FPn [ ]     FScc [ ]     FTST [ ]
//       JMP   [~]     JSR  [x]     NOP [x]     RTD  [!]     RTR  [ ]
//       RTS   [x]     Scc  [x]     TST [ ]
//
//  Pseudo:
//
//          RET [x]
//    TCRETURNj [x]   TCRETURNq [x]
//     TAILJMPj [x]    TAILJMPq [x]
//
//  Map:
//
//   [ ] - was not touched at all
//   [!] - requires extarnal stuff implemented
//   [~] - in progress but usable
//   [x] - done
//
//
//                                   NOTE
//      Though branch and jump instructions are using memory operands they
//      DO NOT read the jump address from memory, they just calculate EA
//      and jump there.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// NOP
//===----------------------------------------------------------------------===//

let hasSideEffects = 0 in {
  def NOP : MxIIII<(outs), (ins), "nop", [], MxEncFixed<0x4E71>>;
}


//===----------------------------------------------------------------------===//
// Conditions
//===----------------------------------------------------------------------===//

/// cc—Carry clear      GE—Greater than or equal
/// LS—Lower or same    PL—Plus
/// CS—Carry set        GT—Greater than
/// LT—Less than        T—Always true*
/// EQ—Equal            HI—Higher
/// MI—Minus            VC—Overflow clear
/// F—Never true*       LE—Less than or equal
/// NE—Not equal        VS—Overflow set
///
/// *Not applicable to the Bcc instructions.
def MxCCt  : MxBead4Bits<0b0000>;
def MxCCf  : MxBead4Bits<0b0001>;
def MxCChi : MxBead4Bits<0b0010>;
def MxCCls : MxBead4Bits<0b0011>;
def MxCCcc : MxBead4Bits<0b0100>;
def MxCCcs : MxBead4Bits<0b0101>;
def MxCCne : MxBead4Bits<0b0110>;
def MxCCeq : MxBead4Bits<0b0111>;
def MxCCvc : MxBead4Bits<0b1000>;
def MxCCvs : MxBead4Bits<0b1001>;
def MxCCpl : MxBead4Bits<0b1010>;
def MxCCmi : MxBead4Bits<0b1011>;
def MxCCge : MxBead4Bits<0b1100>;
def MxCClt : MxBead4Bits<0b1101>;
def MxCCgt : MxBead4Bits<0b1110>;
def MxCCle : MxBead4Bits<0b1111>;

/* def MxCONDeq : PatLeaf <(i8 imm), [{ return N->getZExtValue() == ISD::SETOEQ; }]>; */
/* def MxCONDne : PatLeaf <(i8 imm), [{ return N->getZExtValue() == ISD::SETONE; }]>; */
/* def MxCONDgt : PatLeaf <(i8 imm), [{ return N->getZExtValue() == ISD::SETOGT; }]>; */
/* def MxCONDge : PatLeaf <(i8 imm), [{ return N->getZExtValue() == ISD::SETOGE; }]>; */
/* def MxCONDlt : PatLeaf <(i8 imm), [{ return N->getZExtValue() == ISD::SETOLT; }]>; */
/* def MxCONDle : PatLeaf <(i8 imm), [{ return N->getZExtValue() == ISD::SETOLE; }]>; */

class MxScc<string cc>
    : MxInst<0x0, (outs MxDRD8:$dst), (ins CCRC:$ccr), "s"#cc#"\t$dst">;

foreach cc = [ "cc", "ls", "lt", "eq", "mi", "f", "ne", "ge",
               "pl", "gt", "t", "hi", "vc", "le", "vs"] in
  def SET#cc : MxScc<cc>;

multiclass MMxSETCCPatterns<MxType TYPE, PatFrag FRAG, Instruction SET> {
  // R cc R
  def : Pat<(FRAG TYPE.VT:$lhs, TYPE.VT:$rhs),
            (SET (!cast<Instruction>("CMP"#TYPE.Size#"dd") $lhs, TYPE.ROp:$rhs))>;

  // (An) cc R
  def : Pat<(FRAG (load TYPE.JPat:$lhs), TYPE.VT:$rhs),
            (SET (!cast<Instruction>("CMP"#TYPE.Size#"dj") TYPE.JOp:$lhs, TYPE.ROp:$rhs))>;

  // (i,An) cc R
  def : Pat<(FRAG (load TYPE.PPat:$lhs), TYPE.VT:$rhs),
            (SET (!cast<Instruction>("CMP"#TYPE.Size#"dp") TYPE.POp:$lhs, TYPE.ROp:$rhs))>;

  // (i,An,Xn) cc R
  def : Pat<(FRAG (load TYPE.FPat:$lhs), TYPE.VT:$rhs),
            (SET (!cast<Instruction>("CMP"#TYPE.Size#"df") TYPE.FOp:$lhs, TYPE.ROp:$rhs))>;
}

multiclass MMxSETPatterns<MxType TYPE> {
  // TODO onece float is available...
  // NOTE reread the spec there are some caviats with it like not clearing cc
  /* defm : MMxSETCCPatterns<TYPE, setoeq, SEToeq>; */
  /* defm : MMxSETCCPatterns<TYPE, setogt, SETogt>; */
  /* defm : MMxSETCCPatterns<TYPE, setoge, SEToge>; */
  /* defm : MMxSETCCPatterns<TYPE, setolt, SETolt>; */
  /* defm : MMxSETCCPatterns<TYPE, setole, SETole>; */
  /* defm : MMxSETCCPatterns<TYPE, setone, SETone>; */
  /* defm : MMxSETCCPatterns<TYPE, seto  , SETo>; */
  /* defm : MMxSETCCPatterns<TYPE, setuo , SETuo>; */
  // TODO M680x0 unsinged compare?
  /* defm : MMxSETCCPatterns<TYPE, setueq, SETueq>; */
  /* defm : MMxSETCCPatterns<TYPE, setugt, SETugt>; */
  /* defm : MMxSETCCPatterns<TYPE, setuge, SETuge>; */
  /* defm : MMxSETCCPatterns<TYPE, setult, SETult>; */
  /* defm : MMxSETCCPatterns<TYPE, setule, SETule>; */
  /* defm : MMxSETCCPatterns<TYPE, setune, SETune>; */
  defm : MMxSETCCPatterns<TYPE, seteq, SETeq>;
  defm : MMxSETCCPatterns<TYPE, setne, SETne>;
  defm : MMxSETCCPatterns<TYPE, setgt, SETgt>;
  defm : MMxSETCCPatterns<TYPE, setge, SETge>;
  defm : MMxSETCCPatterns<TYPE, setlt, SETlt>;
  defm : MMxSETCCPatterns<TYPE, setle, SETle>;
}

defm : MMxSETPatterns<MxType8d>;
defm : MMxSETPatterns<MxType16d>;
defm : MMxSETPatterns<MxType32d>;


//===----------------------------------------------------------------------===//
// Jumps
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
// FIXME M680x0 does not allow 16 bit indirect jumps use sext operands
/* def JMP16r     : MxInst<0x0 (outs), (ins M680x0_ARI16:$dst), */
/*                             "jmp\t$dst", */
/*                             [(brind AR16:$dst)]>; */

// jmp (An)
def JMP32j
  : MxInst<0x0, (outs), (ins MxARI32:$dst), "jmp\t$dst", [(brind iPTR:$dst)]>;

}


//===----------------------------------------------------------------------===//
// Branches
//===----------------------------------------------------------------------===//

/// --------------------------------------------------
///  F  E  D  C | B  A  9  8 | 7  6  5  4  3  2  1  0
/// --------------------------------------------------
///  0  1  1  0 | CONDITION |   8-BIT DISPLACEMENT
/// --------------------------------------------------
///  16-BIT DISPLACEMENT IF 8-BIT DISPLACEMENT = $00
/// --------------------------------------------------
///  32-BIT DISPLACEMENT IF 8-BIT DISPLACEMENT = $FF
/// --------------------------------------------------
let isBranch = 1, isTerminator = 1 in
class MxBcc<string cc, Operand TARGET, MxType TYPE, MxEncoding ENC = MxEncEmpty>
    : MxIIII<(outs), (ins CCRC:$ccr, TARGET:$dst), "b"#cc#"."#TYPE.Prefix#"\t$dst", [], ENC>;

foreach cc = [ "cc", "ls", "lt", "eq", "mi", "ne", "ge",
               "pl", "gt", "hi", "vc", "le", "vs"] in {
def B#cc#"8"
  : MxBcc<cc, MxBrTarget8, MxType8,
    MxEncoding<MxBead8Disp<1>, !cast<MxBead4Bits>("MxCC"#cc), MxBead4Bits<0x6>>>;
def B#cc#"16"
  : MxBcc<cc, MxBrTarget16, MxType16,
    MxEncoding<MxBead4Bits<0x0>, MxBead4Bits<0x0>, !cast<MxBead4Bits>("MxCC"#cc),
               MxBead4Bits<0x6>, MxBead16Imm<1>>>;
}

def : Pat<(brcc SETEQ, i8:$lhs, i8:$rhs, bb:$target), (Beq8 (CMP8dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETNE, i8:$lhs, i8:$rhs, bb:$target), (Bne8 (CMP8dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETGT, i8:$lhs, i8:$rhs, bb:$target), (Bgt8 (CMP8dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETGE, i8:$lhs, i8:$rhs, bb:$target), (Bge8 (CMP8dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETLT, i8:$lhs, i8:$rhs, bb:$target), (Blt8 (CMP8dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETLE, i8:$lhs, i8:$rhs, bb:$target), (Ble8 (CMP8dd $lhs, $rhs), MxBrTarget8:$target)>;

def : Pat<(brcc SETEQ, i16:$lhs, i16:$rhs, bb:$target), (Beq8 (CMP16dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETNE, i16:$lhs, i16:$rhs, bb:$target), (Bne8 (CMP16dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETGT, i16:$lhs, i16:$rhs, bb:$target), (Bgt8 (CMP16dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETGE, i16:$lhs, i16:$rhs, bb:$target), (Bge8 (CMP16dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETLT, i16:$lhs, i16:$rhs, bb:$target), (Blt8 (CMP16dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETLE, i16:$lhs, i16:$rhs, bb:$target), (Ble8 (CMP16dd $lhs, $rhs), MxBrTarget8:$target)>;

def : Pat<(brcc SETEQ, i32:$lhs, i32:$rhs, bb:$target), (Beq8 (CMP32dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETNE, i32:$lhs, i32:$rhs, bb:$target), (Bne8 (CMP32dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETGT, i32:$lhs, i32:$rhs, bb:$target), (Bgt8 (CMP32dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETGE, i32:$lhs, i32:$rhs, bb:$target), (Bge8 (CMP32dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETLT, i32:$lhs, i32:$rhs, bb:$target), (Blt8 (CMP32dd $lhs, $rhs), MxBrTarget8:$target)>;
def : Pat<(brcc SETLE, i32:$lhs, i32:$rhs, bb:$target), (Ble8 (CMP32dd $lhs, $rhs), MxBrTarget8:$target)>;

def : Pat<(brcond i8:$value, bb:$target), (Beq8 (CMP8dd $value, 1), MxBrTarget8:$target)>;


/// -------------------------------------------------
///  F  E  D  C  B  A  9  8 | 7  6  5  4  3  2  1  0
/// -------------------------------------------------
///  0  1  1  0  0  0  0  0 |   8-BIT DISPLACEMENT
/// -------------------------------------------------
///  16-BIT DISPLACEMENT IF 8-BIT DISPLACEMENT = $00
/// -------------------------------------------------
///  32-BIT DISPLACEMENT IF 8-BIT DISPLACEMENT = $FF
/// -------------------------------------------------
let isBranch = 1, isTerminator = 1 in
class MxBra<Operand TARGET, MxType TYPE, MxEncoding ENC = MxEncEmpty>
    : MxIIII<(outs), (ins TARGET:$dst), "bra."#TYPE.Prefix#"\t$dst", [], ENC>;

def BRA8  : MxBra<MxBrTarget8, MxType8,
            MxEncoding<MxBead8Disp<0>, MxBead4Bits<0x0>, MxBead4Bits<0x6>>>;
def BRA16 : MxBra<MxBrTarget16, MxType16,
            MxEncoding<MxBead4Bits<0x0>, MxBead4Bits<0x0>,
                       MxBead4Bits<0x0>, MxBead4Bits<0x6>, MxBead16Imm<0>>>;

def : Pat<(br bb:$target), (BRA8 MxBrTarget8:$target)>;


//===----------------------------------------------------------------------===//
// Call
//===----------------------------------------------------------------------===//

class MxCallEncoding<MxEncEA EA, MxEncExt EXT>
    : MxEncoding<EA.Reg, EA.DA, EA.Mode,
                 MxBead2Bits<0b10>, MxBead4Bits<0b1110>, MxBead4Bits<0b0100>,
                 EXT.Imm, EXT.B8, EXT.Scale, EXT.WL, EXT.DAReg>;

let isCall = 1 in
// All calls clobber the non-callee saved registers. %SP is marked as
// a use to prevent stack-pointer assignments that appear immediately
// before calls from potentially appearing dead. Uses for argument
// registers are added manually.
let Uses = [SP] in {

def CALLB : MxIIII<(outs), (ins Mxi32imm:$dst), "jsr\t$dst", [], MxCallEncoding<MxEncEAB,   MxExtI32_0>>;
def CALLq : MxIIII<(outs), (ins MxPCD32:$dst),  "jsr\t$dst", [], MxCallEncoding<MxEncEAq,   MxExtI16_0>>;
def CALLj : MxIIII<(outs), (ins MxARI32:$dst),  "jsr\t$dst", [], MxCallEncoding<MxEncEAj_0, MxExtEmpty>>;

} // let Uses = [SP]

def : Pat<(MxCall (i32 tglobaladdr:$dst)),  (CALLq tglobaladdr:$dst)>,  Requires<[IsPIC]>;
def : Pat<(MxCall (i32 texternalsym:$dst)), (CALLq texternalsym:$dst)>, Requires<[IsPIC]>;
def : Pat<(MxCall (i32 imm:$dst)),          (CALLq imm:$dst)>,          Requires<[IsPIC]>;

def : Pat<(MxCall (i32 tglobaladdr:$dst)),  (CALLB tglobaladdr:$dst)>,  Requires<[IsNotPIC]>;
def : Pat<(MxCall (i32 texternalsym:$dst)), (CALLB texternalsym:$dst)>, Requires<[IsNotPIC]>;
def : Pat<(MxCall (i32 imm:$dst)),          (CALLB imm:$dst)>,          Requires<[IsNotPIC]>;

// NOTE I use iPTR instead of MxCP_ARI so it won't match it for patterns above
// since MxCP_ARI is a single operand pattern. Maybe there is a better solution
def : Pat<(MxCall iPTR:$dst), (CALLj MxARI32:$dst)>, Requires<[IsPIC]>;

//===----------------------------------------------------------------------===//
// Tail Call
//===----------------------------------------------------------------------===//

let isCodeGenOnly = 1 in {
let Uses = [SP] in {
let isCall = 1, isTerminator = 1, isReturn = 1, isBarrier = 1 in {

def TCRETURNq : MxPseudo<(outs), (ins MxPCI32:$dst,    i32imm:$adj)>;
def TAILJMPq  : MxPseudo<(outs), (ins MxPCI32:$dst)>;

// NOTE j does not mean load and jump M680x0 jmp just calculates EA and jumps
// and it is using Mem form like (An) thus j letter.
def TCRETURNj : MxPseudo<(outs), (ins MxARI32_TC:$dst, i32imm:$adj)>;
def TAILJMPj  : MxPseudo<(outs), (ins MxARI32_TC:$dst)>;
}
} // let Uses = [SP]
} // let isCodeGenOnly = 1

//===----------------------------------------------------------------------===//
// Return
//===----------------------------------------------------------------------===//

// TODO don't forget about LINK/UNLK

let isTerminator = 1, isReturn = 1, isBarrier = 1, hasCtrlDep = 1 in {

def RTS : MxIIII<(outs), (ins), "rts", [], MxEncFixed<0x4E75>>;

let isCodeGenOnly = 1 in {
def RET : MxPseudo<(outs), (ins i32imm:$adj, variable_ops), "",
                   [(MxRet timm:$adj)]>;
} // let isCodeGenOnly = 1

}
