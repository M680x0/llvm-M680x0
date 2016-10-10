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
#include "M680x0CallingConv.h"
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

M680x0TargetLowering::M680x0TargetLowering(const M680x0TargetMachine &TM,
                                           const M680x0Subtarget &STI)
    : TargetLowering(TM), Subtarget(STI), ABI(TM.getABI()) {

  setBooleanContents(ZeroOrOneBooleanContent);

  // Use _setjmp/_longjmp instead of setjmp/longjmp.
  setUseUnderscoreSetJmp(true);
  setUseUnderscoreLongJmp(true);

  // Set up the register classes.
  addRegisterClass(MVT::i8,  &M680x0::DR8RegClass);
  addRegisterClass(MVT::i16, &M680x0::XR16RegClass);
  addRegisterClass(MVT::i32, &M680x0::XR32RegClass);

  for (auto VT : MVT::integer_valuetypes())
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i1, Promote);

  for (auto OP : { ISD::SDIV, ISD::UDIV, ISD::SREM, ISD::UREM,
                   ISD::MUL, ISD::MULHS, ISD::MULHU }) {
    setOperationAction(OP, MVT::i8,  Promote);
    setOperationAction(OP, MVT::i16, Legal);
    // TODO this becames legal with newer CPUs
    setOperationAction(OP, MVT::i32, LibCall);
  }

  computeRegisterProperties(STI.getRegisterInfo());

  setMinFunctionAlignment(2); // 2^2 bytes // ??? can it be just 2^1?

}

EVT M680x0TargetLowering::
getSetCCResultType(const DataLayout &DL, LLVMContext& Context, EVT VT) const {
  // M680x0 SETcc producess either 0x00 or 0xFF
  return MVT::i8;
}


SDValue M680x0TargetLowering::LowerOperation(SDValue Op,
                                             SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  default: llvm_unreachable("Should not custom lower this!");
  }
}

#include "M680x0GenCallingConv.inc"

unsigned M680x0TargetLowering::
GetAlignedArgumentStackSize(unsigned StackSize, SelectionDAG& DAG) const {
  const TargetFrameLowering &TFI = *Subtarget.getFrameLowering();
  unsigned StackAlignment = TFI.getStackAlignment();
  uint64_t AlignMask = StackAlignment - 1;
  int64_t Offset = StackSize;
  unsigned SlotSize = Subtarget.getSlotSize();
  if ( (Offset & AlignMask) <= (StackAlignment - SlotSize) ) {
    // Number smaller than 12 so just add the difference.
    Offset += ((StackAlignment - SlotSize) - (Offset & AlignMask));
  } else {
    // Mask out lower bits, add stackalignment once plus the 12 bytes.
    Offset = ((~AlignMask) & Offset) + StackAlignment +
      (StackAlignment-SlotSize);
  }
  return Offset;
}

enum StructReturnType {
  NotStructReturn,
  RegStructReturn,
  StackStructReturn
};

/// Determines whether a function uses struct return semantics.
static StructReturnType
argsAreStructReturn(const SmallVectorImpl<ISD::InputArg> &Ins) {
  if (Ins.empty())
    return NotStructReturn;

  const ISD::ArgFlagsTy &Flags = Ins[0].Flags;
  if (!Flags.isSRet())
    return NotStructReturn;
  if (Flags.isInReg())
    return RegStructReturn;
  return StackStructReturn;
}

/// Return true if the calling convention is one that we can guarantee TCO for.
static bool canGuaranteeTCO(CallingConv::ID CC) {
  return false; // FIXME well, i need to verify if current CC can do this
}

/// Return true if the function is being made into a tailcall target by
/// changing its ABI.
static bool shouldGuaranteeTCO(CallingConv::ID CC, bool GuaranteedTailCallOpt) {
  return GuaranteedTailCallOpt && canGuaranteeTCO(CC);
}

SDValue M680x0TargetLowering::
LowerMemArgument(SDValue Chain, CallingConv::ID CallConv,
                 const SmallVectorImpl<ISD::InputArg> &Ins,
                 const SDLoc &DL, SelectionDAG &DAG, const CCValAssign &VA,
                 MachineFrameInfo &MFI, unsigned i) const {
  // Create the nodes corresponding to a load from this parameter slot.
  ISD::ArgFlagsTy Flags = Ins[i].Flags;
  EVT ValVT;

  // If value is passed by pointer we have address passed instead of the value
  // itself.
  if (VA.getLocInfo() == CCValAssign::Indirect)
    ValVT = VA.getLocVT();
  else
    ValVT = VA.getValVT();

  // Calculate SP offset of interrupt parameter, re-arrange the slot normally
  // taken by a return address.
  // TODO interrupts
  // int Offset = 0;
  // if (CallConv == CallingConv::M680x0_INTR) {
  //   const M680x0Subtarget& Subtarget =
  //       static_cast<const M680x0Subtarget&>(DAG.getSubtarget());
  //   // M680x0 interrupts may take one or two arguments.
  //   // On the stack there will be no return address as in regular call.
  //   // Offset of last argument need to be set to -4/-8 bytes.
  //   // Where offset of the first argument out of two, should be set to 0 bytes.
  //   Offset = (Subtarget.is64Bit() ? 8 : 4) * ((i + 1) % Ins.size() - 1);
  // }

  // FIXME: For now, all byval parameter objects are marked mutable. This can be
  // changed with more analysis.
  // In case of tail call optimization mark all arguments mutable. Since they
  // could be overwritten by lowering of arguments in case of a tail call.
  bool AlwaysUseMutable = shouldGuaranteeTCO(
      CallConv, DAG.getTarget().Options.GuaranteedTailCallOpt);
  bool isImmutable = !AlwaysUseMutable && !Flags.isByVal();

  if (Flags.isByVal()) {
    unsigned Bytes = Flags.getByValSize();
    if (Bytes == 0) Bytes = 1; // Don't create zero-sized stack objects.
    int FI = MFI.CreateFixedObject(Bytes, VA.getLocMemOffset(), isImmutable);
    // Adjust SP offset of interrupt parameter.
    // TODO interrupts
    // if (CallConv == CallingConv::M680x0_INTR) {
    //   MFI.setObjectOffset(FI, Offset);
    // }
    return DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));
  } else {
    int FI = MFI.CreateFixedObject(ValVT.getSizeInBits()/8,
                                   VA.getLocMemOffset(), isImmutable);

    // Set SExt or ZExt flag.
    if (VA.getLocInfo() == CCValAssign::ZExt) {
      MFI.setObjectZExt(FI, true);
    } else if (VA.getLocInfo() == CCValAssign::SExt) {
      MFI.setObjectSExt(FI, true);
    }

    // Adjust SP offset of interrupt parameter.
    // TODO interrupts
    // if (CallConv == CallingConv::M680x0_INTR) {
    //   MFI.setObjectOffset(FI, Offset);
    // }

    SDValue FIN = DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));
    SDValue Val = DAG.getLoad(
        ValVT, DL, Chain, FIN,
        MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI));
    return VA.isExtInLoc() ?
      DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Val) : Val;
  }
}

//===----------------------------------------------------------------------===//
//            Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue M680x0TargetLowering::
LowerFormalArguments(SDValue Chain, CallingConv::ID CCID, bool isVarArg,
                     const SmallVectorImpl<ISD::InputArg> &Ins,
                     const SDLoc &DL, SelectionDAG &DAG,
                     SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();
  M680x0MachineFunctionInfo *MMFI = MF.getInfo<M680x0MachineFunctionInfo>();
  // const TargetFrameLowering &TFL = *Subtarget.getFrameLowering();

  MachineFrameInfo &MFI = MF.getFrameInfo();

  // TODO interrupts...
  // if (CCID == CallingConv::M680x0_INTR) {
  //   bool isLegal = Ins.size() == 1 ||
  //                  (Ins.size() == 2 && ((Is64Bit && Ins[1].VT == MVT::i64) ||
  //                                       (!Is64Bit && Ins[1].VT == MVT::i32)));
  //   if (!isLegal)
  //     report_fatal_error("M680x0 interrupts may take one or two arguments");
  // }

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CCID, isVarArg, MF, ArgLocs, *DAG.getContext());

  CCInfo.AnalyzeFormalArguments(Ins, CC_M680x0);

  unsigned LastVal = ~0U;
  SDValue ArgValue;
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    assert(VA.getValNo() != LastVal && "Same value in different locations");

    LastVal = VA.getValNo();

    if (VA.isRegLoc()) {
      EVT RegVT = VA.getLocVT();
      const TargetRegisterClass *RC;
      if (RegVT == MVT::i32)
        RC = &M680x0::XR32RegClass;
      else
        llvm_unreachable("Unknown argument type!");

      unsigned Reg = MF.addLiveIn(VA.getLocReg(), RC);
      ArgValue = DAG.getCopyFromReg(Chain, DL, Reg, RegVT);

      // If this is an 8 or 16-bit value, it is really passed promoted to 32
      // bits.  Insert an assert[sz]ext to capture this, then truncate to the
      // right size.
      if (VA.getLocInfo() == CCValAssign::SExt) {
        ArgValue = DAG.getNode(ISD::AssertSext, DL, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
      } else if (VA.getLocInfo() == CCValAssign::ZExt) {
        ArgValue = DAG.getNode(ISD::AssertZext, DL, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
      } else if (VA.getLocInfo() == CCValAssign::BCvt) {
        ArgValue = DAG.getBitcast(VA.getValVT(), ArgValue);
      }

      if (VA.isExtInLoc()) {
          ArgValue = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), ArgValue);
      }
    } else {
      assert(VA.isMemLoc());
      ArgValue = LowerMemArgument(Chain, CCID, Ins, DL, DAG, VA, MFI, i);
    }

    // If value is passed via pointer - do a load.
    // TODO debug how this really works
    // ??? May I remove this indirect shizzle?
    if (VA.getLocInfo() == CCValAssign::Indirect)
      ArgValue = DAG.getLoad(VA.getValVT(), DL, Chain,
                             ArgValue, MachinePointerInfo());

    InVals.push_back(ArgValue);
  }

  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    // Swift calling convention does not require we copy the sret argument
    // into %D0 for the return. We don't set SRetReturnReg for Swift.
    if (CCID == CallingConv::Swift)
      continue;

    // ABI require that for returning structs by value we copy the sret argument
    // into %D0 for the return. Save the argument into a virtual register so that
    // we can access it from the return points.
    if (Ins[i].Flags.isSRet()) {
      unsigned Reg = MMFI->getSRetReturnReg();
      if (!Reg) {
        MVT PtrTy = getPointerTy(DAG.getDataLayout());
        Reg = MF.getRegInfo().createVirtualRegister(getRegClassFor(PtrTy));
        MMFI->setSRetReturnReg(Reg);
      }
      SDValue Copy = DAG.getCopyToReg(DAG.getEntryNode(), DL, Reg, InVals[i]);
      Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, Copy, Chain);
      break;
    }
  }

  unsigned StackSize = CCInfo.getNextStackOffset();
  // Align stack specially for tail calls.
  if (shouldGuaranteeTCO(CCID, MF.getTarget().Options.GuaranteedTailCallOpt))
    StackSize = GetAlignedArgumentStackSize(StackSize, DAG);

  // If the function takes variable number of arguments, make a frame index for
  // the start of the first vararg value... for expansion of llvm.va_start. We
  // can skip this if there are no va_start calls.
  // if (MFI.hasVAStart() &&
  //     (Is64Bit || (CCID != CallingConv::M680x0_FastCall &&
  //                  CCID != CallingConv::M680x0_ThisCall))) {
    MMFI->setVarArgsFrameIndex(MFI.CreateFixedObject(1, StackSize, true));
  // }

  if (isVarArg && MFI.hasMustTailInVarArgFunc()) {
    // We forward some GPRs and some vector types.
    SmallVector<MVT, 2> RegParmTypes;
    MVT IntVT = MVT::i32;
    RegParmTypes.push_back(IntVT);

    // Compute the set of forwarded registers. The rest are scratch.
    // ??? what is this for?
    SmallVectorImpl<ForwardedRegister> &Forwards =
        MMFI->getForwardedMustTailRegParms();
    CCInfo.analyzeMustTailForwardedRegisters(Forwards, RegParmTypes, CC_M680x0);

    // Copy all forwards from physical to virtual registers.
    for (ForwardedRegister &F : Forwards) {
      // FIXME: Can we use a less constrained schedule?
      SDValue RegVal = DAG.getCopyFromReg(Chain, DL, F.VReg, F.VT);
      F.VReg = MF.getRegInfo().createVirtualRegister(getRegClassFor(F.VT));
      Chain = DAG.getCopyToReg(Chain, DL, F.VReg, RegVal);
    }
  }

  // Some CCs need callee pop.
  if (M680x0::isCalleePop(CCID, isVarArg,
                       MF.getTarget().Options.GuaranteedTailCallOpt)) {
    MMFI->setBytesToPopOnReturn(StackSize); // Callee pops everything.
  // } else if (CCID == CallingConv::M680x0_INTR && Ins.size() == 2) {
  //   // M680x0 interrupts must pop the error code if present
  //   MMFI->setBytesToPopOnReturn(4);
  } else {
    MMFI->setBytesToPopOnReturn(0); // Callee pops nothing.
    // If this is an sret function, the return should pop the hidden pointer.
    if (!canGuaranteeTCO(CCID) && argsAreStructReturn(Ins) == StackStructReturn)
      MMFI->setBytesToPopOnReturn(4);
  }

  // if (!Is64Bit) {
  //   // RegSaveFrameIndex is M680x0-64 only.
  //   MMFI->setRegSaveFrameIndex(0xAAAAAAA);
  //   if (CCID == CallingConv::M680x0_FastCall ||
  //       CCID == CallingConv::M680x0_ThisCall)
  //     // fastcc functions can't have varargs.
  //     MMFI->setVarArgsFrameIndex(0xAAAAAAA);
  // }

  MMFI->setArgumentStackSize(StackSize);

  return Chain;
}

//===----------------------------------------------------------------------===//
//              Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue M680x0TargetLowering::
LowerReturn(SDValue Chain, CallingConv::ID CCID, bool isVarArg,
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
  // into %d0 for the return, and SRetReturnReg is not set for Swift.

  // ABI require that for returning structs by value we copy the sret argument
  // into %D0 for the return. Save the argument into a virtual register so that
  // we can access it from the return points.
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

const char *M680x0TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case M680x0ISD::CALL:      return "M680x0ISD::CALL";
  case M680x0ISD::TAIL_CALL: return "M680x0ISD::TAIL_CALL";
  case M680x0ISD::GP_REL:    return "M680x0ISD::GP_REL";
  case M680x0ISD::RET:       return "M680x0ISD::RET";
  case M680x0ISD::TC_RETURN: return "M680x0ISD::TC_RETURN";
  case M680x0ISD::ADD:       return "M680x0ISD::ADD";
  case M680x0ISD::SUB:       return "M680x0ISD::SUB";
  case M680x0ISD::ADDX:      return "M680x0ISD::ADDX";
  case M680x0ISD::SUBX:      return "M680x0ISD::SUBX";
  case M680x0ISD::SMUL:      return "M680x0ISD::SMUL";
  case M680x0ISD::INC:       return "M680x0ISD::INC";
  case M680x0ISD::DEC:       return "M680x0ISD::DEC";
  case M680x0ISD::OR:        return "M680x0ISD::OR";
  case M680x0ISD::XOR:       return "M680x0ISD::XOR";
  case M680x0ISD::AND:       return "M680x0ISD::AND";
  case M680x0ISD::Wrapper:   return "M680x0ISD::Wrapper";
  default:                   return NULL;
  }
}

/// Determines whether the callee is required to pop its own arguments.
/// Callee pop is necessary to support tail calls.
bool M680x0::isCalleePop(CallingConv::ID CallingConv,
                         bool IsVarArg, bool GuaranteeTCO) {
  // FIXME RTD is not available untill M68010
  return false;
  // // If GuaranteeTCO is true, we force some calls to be callee pop so that we
  // // can guarantee TCO.
  // if (!IsVarArg && shouldGuaranteeTCO(CallingConv, GuaranteeTCO))
  //   return true;
  //
  // switch (CallingConv) {
  // default:
  //   return false;
  // case CallingConv::M680x0_StdCall:
  // case CallingConv::M680x0_FastCall:
  // case CallingConv::M680x0_ThisCall:
  // case CallingConv::M680x0_VectorCall:
  //   return !is64Bit;
  // }
}
