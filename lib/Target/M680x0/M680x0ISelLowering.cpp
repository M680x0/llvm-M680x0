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

  // Set up the register classes.
  addRegisterClass(MVT::i8,  &M680x0::DR8RegClass);
  addRegisterClass(MVT::i16, &M680x0::ALL16RegClass);
  addRegisterClass(MVT::i32, &M680x0::ALL32RegClass);

  computeRegisterProperties(STI.getRegisterInfo());

  setMinFunctionAlignment(2); // 2^2 bytes // HMM... can it be just 2^1?

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
LowerFormalArguments(SDValue Chain, CallingConv::ID CCID, bool isVarArg,
                     const SmallVectorImpl<ISD::InputArg> &Ins,
                     const SDLoc &DL, SelectionDAG &DAG,
                     SmallVectorImpl<SDValue> &InVals) const {
  return Chain;
}

//===----------------------------------------------------------------------===//
//              Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue
M680x0TargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CCID,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::OutputArg> &Outs,
                               const SmallVectorImpl<SDValue> &OutVals,
                               const SDLoc &DL, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  M680x0MachineFunctionInfo *MFI = MF.getInfo<M680x0MachineFunctionInfo>();

  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CCID, isVarArg, MF, RVLocs, *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_M680x0);

  SDValue Flag;
  SmallVector<SDValue, 6> RetOps;
  // Operand #0 = Chain (updated below)
  RetOps.push_back(Chain);
  // Operand #1 = Bytes To Pop
  RetOps.push_back(DAG.getTargetConstant(MFI->getBytesToPopOnReturn(),
                                         DL, MVT::i32));

  // Copy the result values into the output registers.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");
    SDValue ValToCopy = OutVals[i];
    EVT ValVT = ValToCopy.getValueType();

    // Promote values to the appropriate types.
    if (VA.getLocInfo() == CCValAssign::SExt)
      ValToCopy = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), ValToCopy);
    else if (VA.getLocInfo() == CCValAssign::ZExt)
      ValToCopy = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), ValToCopy);
    else if (VA.getLocInfo() == CCValAssign::AExt) {
      if (ValVT.isVector() && ValVT.getVectorElementType() == MVT::i1)
        ValToCopy = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), ValToCopy);
      else
        ValToCopy = DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), ValToCopy);
    }
    else if (VA.getLocInfo() == CCValAssign::BCvt)
      ValToCopy = DAG.getBitcast(VA.getLocVT(), ValToCopy);

    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), ValToCopy, Flag);
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  // Swift calling convention does not require we copy the sret argument
  // into %rax/%eax for the return, and SRetReturnReg is not set for Swift.

  // All M680x0 ABIs require that for returning structs by value we copy
  // the sret argument into %rax/%eax (depending on ABI) for the return.
  // We saved the argument into a virtual register in the entry block,
  // so now we copy the value out and into %rax/%eax.
  //
  // Checking Function.hasStructRetAttr() here is insufficient because the IR
  // may not have an explicit sret argument. If MFI.CanLowerReturn is
  // false, then an sret argument may be implicitly inserted in the SelDAG. In
  // either case MFI->setSRetReturnReg() will have been called.
  if (unsigned SRetReg = MFI->getSRetReturnReg()) {
    // ??? Can i just move this to the top and escape this explanation?
    // When we have both sret and another return value, we should use the
    // original Chain stored in RetOps[0], instead of the current Chain updated
    // in the above loop. If we only have sret, RetOps[0] equals to Chain.

    // For the case of sret and another return value, we have
    //   Chain_0 at the function entry
    //   Chain_1 = getCopyToReg(Chain_0) in the above loop
    // If we use Chain_1 in getCopyFromReg, we will have
    //   Val = getCopyFromReg(Chain_1)
    //   Chain_2 = getCopyToReg(Chain_1, Val) from below

    // getCopyToReg(Chain_0) will be glued together with
    // getCopyToReg(Chain_1, Val) into Unit A, getCopyFromReg(Chain_1) will be
    // in Unit B, and we will have cyclic dependency between Unit A and Unit B:
    //   Data dependency from Unit B to Unit A due to usage of Val in
    //     getCopyToReg(Chain_1, Val)
    //   Chain dependency from Unit A to Unit B

    // So here, we use RetOps[0] (i.e Chain_0) for getCopyFromReg.
    SDValue Val = DAG.getCopyFromReg(RetOps[0], DL, SRetReg,
                                     getPointerTy(MF.getDataLayout()));

    // ??? How will this work if CC does not use registers for args passing?
    // ??? What if I return multiple structs?
    unsigned RetValReg = M680x0::D0;
    Chain = DAG.getCopyToReg(Chain, DL, RetValReg, Val, Flag);
    Flag = Chain.getValue(1);

    RetOps.push_back(
        DAG.getRegister(RetValReg, getPointerTy(DAG.getDataLayout())));
  }

  // ??? What is it doing?
  // const M680x0RegisterInfo *TRI = Subtarget.getRegisterInfo();
  // const MCPhysReg *I =
  //     TRI->getCalleeSavedRegsViaCopy(&DAG.getMachineFunction());
  // if (I) {
  //   for (; *I; ++I) {
  //     if (M680x0::GR64RegClass.contains(*I))
  //       RetOps.push_back(DAG.getRegister(*I, MVT::i64));
  //     else
  //       llvm_unreachable("Unexpected register class in CSRsViaCopy!");
  //   }
  // }

  RetOps[0] = Chain;  // Update chain.

  // Add the flag if we have it.
  if (Flag.getNode())
    RetOps.push_back(Flag);

  return DAG.getNode(M680x0ISD::RET, DL, MVT::Other, RetOps);
}
