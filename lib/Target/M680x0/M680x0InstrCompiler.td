//===-- M680x0InstrCompiler.td - Compiler Pseudos and Patterns tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the various pseudo instructions used by the compiler,
// as well as Pat patterns used during instruction selection.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// DAG Pattern Matching Rules
//===----------------------------------------------------------------------===//

// ConstantPool GlobalAddress, ExternalSymbol, and JumpTable
def : Pat<(i32 (MxWrapper tconstpool    :$dst)), (MOV32ri tconstpool    :$dst)>;
def : Pat<(i32 (MxWrapper tglobaladdr   :$dst)), (MOV32ri tglobaladdr   :$dst)>;
def : Pat<(i32 (MxWrapper texternalsym  :$dst)), (MOV32ri texternalsym  :$dst)>;
def : Pat<(i32 (MxWrapper tjumptable    :$dst)), (MOV32ri tjumptable    :$dst)>;
def : Pat<(i32 (MxWrapper tblockaddress :$dst)), (MOV32ri tblockaddress :$dst)>;

def : Pat<(add MxDRD32:$src, (MxWrapper tconstpool:$opd)),
          (ADD32ri MxDRD32:$src, tconstpool:$opd)>;
def : Pat<(add MxARD32:$src, (MxWrapper tjumptable:$opd)),
          (ADD32ri MxARD32:$src, tjumptable:$opd)>;
def : Pat<(add MxARD32:$src, (MxWrapper tglobaladdr :$opd)),
          (ADD32ri MxARD32:$src, tglobaladdr:$opd)>;
def : Pat<(add MxARD32:$src, (MxWrapper texternalsym:$opd)),
          (ADD32ri MxARD32:$src, texternalsym:$opd)>;
def : Pat<(add MxARD32:$src, (MxWrapper tblockaddress:$opd)),
          (ADD32ri MxARD32:$src, tblockaddress:$opd)>;

def : Pat<(store (i32 (MxWrapper tglobaladdr:$src)), iPTR:$dst),
          (MOV32ji MxARI32:$dst, tglobaladdr:$src)>;
def : Pat<(store (i32 (MxWrapper texternalsym:$src)), iPTR:$dst),
          (MOV32ji MxARI32:$dst, texternalsym:$src)>;
def : Pat<(store (i32 (MxWrapper tblockaddress:$src)), iPTR:$dst),
          (MOV32ji MxARI32:$dst, tblockaddress:$src)>;


//===----------------------------------------------------------------------===//
// Calls
//===----------------------------------------------------------------------===//

// ADJCALLSTACKDOWN/UP implicitly use/def %SP because they may be expanded into
// a stack adjustment and the codegen must know that they may modify the stack
// pointer before prolog-epilog rewriting occurs.
// Pessimistically assume ADJCALLSTACKDOWN / ADJCALLSTACKUP will become
// sub / add which can clobber CCR.
let Defs = [SP, CCR], Uses = [SP] in {

def ADJCALLSTACKDOWN
  : MxPseudo<(outs), (ins i32imm:$amt1), "#ADJCALLSTACKDOWN",
            [(MxCallSeqStart timm:$amt1)]>;

def ADJCALLSTACKUP
  : MxPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2), "#ADJCALLSTACKUP",
            [(MxCallSeqEnd timm:$amt1, timm:$amt2)]>;

} // Defs

//===----------------------------------------------------------------------===//
// Tail Call
//===----------------------------------------------------------------------===//

// FIXME need to read more about TC in llvm

// Tailcall stuff. The TCRETURN instructions execute after the epilog, so they
// can never use callee-saved registers. That is the purpose of the XR32_TC
// register classes.

// FIXME: This is disabled for PIC mode because the global base
// register which is part of the address mode may be assigned a
// callee-saved register.
def : Pat<(MxTCRet (load MxCP_ARII:$dst), imm:$adj),
          (TCRETURNj (MOV32af_TC MxARII32:$dst), imm:$adj)>,
      Requires<[IsNotPIC]>;

def : Pat<(MxTCRet AR32_TC:$dst, imm:$adj),
          (TCRETURNj MxARI32_TC:$dst, imm:$adj)>;

def : Pat<(MxTCRet (i32 tglobaladdr:$dst), imm:$adj),
          (TCRETURNq MxPCI32:$dst, imm:$adj)>;

def : Pat<(MxTCRet (i32 texternalsym:$dst), imm:$adj),
          (TCRETURNq MxPCI32:$dst, imm:$adj)>;
