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
  def NOP : MxInst<0x4E71, (outs), (ins), "nop", []>;
}

// Indirect branching
let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
// FIXME M680x0 does not allow 16 bit indirect jumps use sext operands
/* def JMP16r     : MxInst<0x0 (outs), (ins M680x0_ARI16:$dst), */
/*                             "jmp\t$dst", */
/*                             [(brind AR16:$dst)]>; */

// jmp (An)
def JMP32j : MxInst<0x0, (outs), (ins MxARI32:$dst), "jmp\t$dst",
                    [(brind iPTR:$dst)]>;
}

// TODO don't forget about LINK/UNLK
// Return
let isTerminator = 1, isReturn = 1, isBarrier = 1, hasCtrlDep = 1 in {

def RTS : MxInst<0x0, (outs), (ins), "rts", []>;

let isCodeGenOnly = 1 in {
def RET : MxPseudo<(outs), (ins i32imm:$adj, variable_ops), "",
                   [(MxRet timm:$adj)]>;
} // let isCodeGenOnly = 1

}

// Tail call stuff.
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

