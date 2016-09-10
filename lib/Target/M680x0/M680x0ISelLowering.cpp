//===-- M680x0ISelLowering.cpp - M680x0 DAG Lowering Implementation -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that M680x0 uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//
#include "M680x0ISelLowering.h"

#include "M680x0MachineFunction.h"
#include "M680x0TargetMachine.h"
#include "M680x0TargetObjectFile.h"
#include "M680x0Subtarget.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "M680x0-isel"

const char *M680x0TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case M680x0ISD::CALL:      return "M680x0ISD::CALL";
  case M680x0ISD::TAIL_CALL: return "M680x0ISD::TAIL_CALL";
  case M680x0ISD::GP_REL:    return "M680x0ISD::GP_REL";
  case M680x0ISD::RET:       return "M680x0ISD::RET";
  case M680x0ISD::EH_RETURN: return "M680x0ISD::EH_RETURN";
  case M680x0ISD::DivRem:    return "M680x0ISD::DivRem";
  case M680x0ISD::DivRemU:   return "M680x0ISD::DivRemU";
  case M680x0ISD::Wrapper:   return "M680x0ISD::Wrapper";
  default:                   return NULL;
  }
}

M680x0TargetLowering::M680x0TargetLowering(const M680x0TargetMachine &TM,
                                           const M680x0Subtarget &STI)
    : TargetLowering(TM), Subtarget(STI), ABI(TM.getABI()) {

}

SDValue M680x0TargetLowering::LowerOperation(SDValue Op,
                                             SelectionDAG &DAG) const {

  return Op;
}

#include "M680x0GenCallingConv.inc"

//===----------------------------------------------------------------------===//
//            Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//

/// LowerFormalArguments - transform physical registers into virtual registers
/// and generate load operations for arguments places on the stack.
SDValue M680x0TargetLowering::
LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                     const SmallVectorImpl<ISD::InputArg> &Ins,
                     const SDLoc &DL, SelectionDAG &DAG,
                     SmallVectorImpl<SDValue> &InVals) const {
  return Chain;
}

//===----------------------------------------------------------------------===//
//              Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue M680x0TargetLowering::
LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                    const SmallVectorImpl<ISD::OutputArg> &Outs,
                    const SmallVectorImpl<SDValue> &OutVals,
                    const SDLoc &DL, SelectionDAG &DAG) const {
  return DAG.getNode(M680x0ISD::RET, DL, MVT::Other, Chain,
                     DAG.getRegister(M680x0::D0, MVT::i32));
}
