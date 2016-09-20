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
//
//===----------------------------------------------------------------------===//

// Indirect branching
let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
// FIXME M680x0 does not allow 16 bit indirect jumps use sext operands
/* def JMP16r     : MxInst<0x0 (outs), (ins M680x0_ARI16:$dst), */
/*                             "jmp\t$dst", */
/*                             [(brind AR16:$dst)]>; */

// jmp (An)
def JMP32r : MxInst<0x0, (outs), (ins MxARI32:$dst), "jmp\t$dst",
                    [(brind iPTR:$dst)]>;

// FIXME how to jmp from mem?
/* def JMP16m     : MxInst<0x0 (outs), (ins M680x0_ARI16:$dst), */
/*                             "jmp{w}\t{*}$dst", */
/*                             [(brind (M680x0_loadi16 AR32:$dst))]>; */

/* def JMP32m     : MxInst<0x0 (outs), (ins M680x0_ARI32:$dst), */
/*                             "jmp\t$dst", */
/*                             [(brind (M680x0_loadi32 AR32:$dst))]>; */
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

def TCRETURNdi : MxPseudo<(outs), (ins Mxi32_PCRel:$dst, i32imm:$offset)>;
def TCRETURNri : MxPseudo<(outs), (ins AR32_TC:$dst,     i32imm:$offset)>;

let mayLoad = 1 in
def TCRETURNmi : MxPseudo<(outs), (ins MxARII32_TC:$dst,  i32imm:$offset)>;

def TAILJMPd : MxPseudo<(outs), (ins Mxi32_PCRel:$dst)>;
def TAILJMPr : MxPseudo<(outs), (ins MxARI32_TC:$dst)>;

let mayLoad = 1 in
def TAILJMPm : MxPseudo<(outs), (ins MxARII32_TC:$dst)>;
}
} // let Uses = [SP]
} // let isCodeGenOnly = 1

