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
//       BRA   [ ]     BSR  [ ]     Bcc [ ]     DBcc [ ]     FBcc [ ]
//       FDBcc [ ]     FNOP [ ]     FPn [ ]     FScc [ ]     FTST [ ]
//       JMP   [~]     JSR  [ ]     NOP [x]     RTD  [ ]     RTR  [ ]
//       RTS   [x]     Scc  [ ]     TST [ ]
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

// NOP
let hasSideEffects = 0 in {
  def NOP : MxInstruction<(outs), (ins), "nop", [], MxEncFixed<0x4E71>>;
}

//===----------------------------------------------------------------------===//
// Conditions
//===----------------------------------------------------------------------===//

class MxScc<string cc>
    : MxInst<0x0, (outs MxDRD8:$dst), (ins CCRC:$ccr), "s"#cc#"\t$dst">;

///
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
  defm : MMxSETCCPatterns<TYPE, setgt, SETgt>;
  defm : MMxSETCCPatterns<TYPE, setge, SETge>;
  defm : MMxSETCCPatterns<TYPE, setlt, SETlt>;
  defm : MMxSETCCPatterns<TYPE, setle, SETle>;
  defm : MMxSETCCPatterns<TYPE, setne, SETne>;
}

defm : MMxSETPatterns<MxType8d>;
defm : MMxSETPatterns<MxType16d>;
defm : MMxSETPatterns<MxType32d>;

//===----------------------------------------------------------------------===//
// Branches
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
// FIXME M680x0 does not allow 16 bit indirect jumps use sext operands
/* def JMP16r     : MxInst<0x0 (outs), (ins M680x0_ARI16:$dst), */
/*                             "jmp\t$dst", */
/*                             [(brind AR16:$dst)]>; */

// jmp (An)
def JMP32j : MxInst<0x0, (outs), (ins MxARI32:$dst), "jmp\t$dst",
                    [(brind iPTR:$dst)]>;
}

let isBranch = 1, isTerminator = 1, Uses = [CCR] in
class MxBcc<string cc, Operand brtarget>
    : MxInst<0x0, (outs), (ins brtarget:$dst), "b"#cc#"\t$dst">;

///
/// cc—Carry clear      GE—Greater than or equal
/// LS—Lower or same    PL—Plus
/// CS—Carry set        GT—Greater than
/// LT—Less than
/// EQ—Equal            HI—Higher
/// MI—Minus            VC—Overflow clear
///                     LE—Less than or equal
/// NE—Not equal        VS—Overflow set
foreach cc = [ "cc", "ls", "lt", "eq", "mi", "ne", "ge",
               "pl", "gt", "hi", "vc", "le", "vs"] in {
  def B#cc#"8"  : MxBcc<cc, MxBrTarget8>;
  def B#cc#"16" : MxBcc<cc, MxBrTarget16>;
}


//===----------------------------------------------------------------------===//
// Call
//===----------------------------------------------------------------------===//

let isCall = 1 in
// All calls clobber the non-callee saved registers. %SP is marked as
// a use to prevent stack-pointer assignments that appear immediately
// before calls from potentially appearing dead. Uses for argument
// registers are added manually.
let Uses = [SP] in {

def CALLq : MxInst<0x0, (outs), (ins MxPCD32:$dst), "jsr\t$dst">;
def CALLa : MxInst<0x0, (outs), (ins MxARI32:$dst), "jsr\t$dst">;

} // let Uses = [SP]

def : Pat<(MxCall (i32 tglobaladdr:$dst)),  (CALLq tglobaladdr:$dst)>;
def : Pat<(MxCall (i32 texternalsym:$dst)), (CALLq texternalsym:$dst)>;
def : Pat<(MxCall (i32 imm:$dst)),          (CALLq imm:$dst)>;
// NOTE I use iPTR instead of MxCP_ARI so it won't match it for patterns above
// since MxCP_ARI is a single operand pattern. Maybe there is a better solution
def : Pat<(MxCall iPTR:$dst),           (CALLa MxARI32:$dst)>;

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

def RTS : MxInstruction<(outs), (ins), "rts", [], MxEncFixed<0x4E75>>;

let isCodeGenOnly = 1 in {
def RET : MxPseudo<(outs), (ins i32imm:$adj, variable_ops), "",
                   [(MxRet timm:$adj)]>;
} // let isCodeGenOnly = 1

}
