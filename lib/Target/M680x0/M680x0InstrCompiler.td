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

// FIXME need to read more about TC in llvm

// Tailcall stuff. The TCRETURN instructions execute after the epilog, so they
// can never use callee-saved registers. That is the purpose of the XR32_TC
// register classes.

// FIXME: This is disabled for PIC mode because the global base
// register which is part of the address mode may be assigned a
// callee-saved register.
def : Pat<(MxTCRet (load MxCP_ARII:$dst), imm:$off),
          (TCRETURNmi MxCP_ARII:$dst, imm:$off)>,
      Requires<[IsNotPIC]>;

def : Pat<(MxTCRet AR32_TC:$dst, imm:$off),
          (TCRETURNri AR32_TC:$dst, imm:$off)>;

def : Pat<(MxTCRet (i32 tglobaladdr:$dst), imm:$off),
          (TCRETURNdi tglobaladdr:$dst, imm:$off)>;

def : Pat<(MxTCRet (i32 texternalsym:$dst), imm:$off),
          (TCRETURNdi texternalsym:$dst, imm:$off)>;
