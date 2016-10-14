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

STATISTIC(NumTailCalls, "Number of tail calls");

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

enum StructReturnType {
  NotStructReturn,
  RegStructReturn,
  StackStructReturn
};

static StructReturnType
callIsStructReturn(const SmallVectorImpl<ISD::OutputArg> &Outs) {
  if (Outs.empty())
    return NotStructReturn;

  const ISD::ArgFlagsTy &Flags = Outs[0].Flags;
  if (!Flags.isSRet())
    return NotStructReturn;
  if (Flags.isInReg())
    return RegStructReturn;
  return StackStructReturn;
}

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

/// Make a copy of an aggregate at address specified by "Src" to address
/// "Dst" with size and alignment information specified by the specific
/// parameter attribute. The copy will be passed as a byval function parameter.
static SDValue
CreateCopyOfByValArgument(SDValue Src, SDValue Dst, SDValue Chain,
                          ISD::ArgFlagsTy Flags, SelectionDAG &DAG,
                          const SDLoc &DL) {
  SDValue SizeNode = DAG.getConstant(Flags.getByValSize(), DL, MVT::i32);

  return DAG.getMemcpy(Chain, DL, Dst, Src, SizeNode, Flags.getByValAlign(),
                       /*isVolatile*/false, /*AlwaysInline=*/true,
                       /*isTailCall*/false,
                       MachinePointerInfo(), MachinePointerInfo());
}

/// Return true if the calling convention is one that we can guarantee TCO for.
static bool canGuaranteeTCO(CallingConv::ID CC) {
  return false;
  // return CC == CallingConv::Fast; // TODO Since M68010 only
}

/// Return true if we might ever do TCO for calls with this calling convention.
static bool mayTailCallThisCC(CallingConv::ID CC) {
  switch (CC) {
  // C calling conventions:
  case CallingConv::C:
    return true;
  default:
    return canGuaranteeTCO(CC);
  }
}

/// Return true if the function is being made into a tailcall target by
/// changing its ABI.
static bool shouldGuaranteeTCO(CallingConv::ID CC, bool GuaranteedTailCallOpt) {
  return GuaranteedTailCallOpt && canGuaranteeTCO(CC);
}

/// Return true if the given stack call argument is already available in the
/// same position (relatively) of the caller's incoming argument stack.
static bool
MatchingStackOffset(SDValue Arg, unsigned Offset, ISD::ArgFlagsTy Flags,
                    MachineFrameInfo &MFI, const MachineRegisterInfo *MRI,
                    const M680x0InstrInfo *TII, const CCValAssign &VA) {
  unsigned Bytes = Arg.getValueType().getSizeInBits() / 8;

  for (;;) {
    // Look through nodes that don't alter the bits of the incoming value.
    unsigned Op = Arg.getOpcode();
    if (Op == ISD::ZERO_EXTEND || Op == ISD::ANY_EXTEND || Op == ISD::BITCAST) {
      Arg = Arg.getOperand(0);
      continue;
    }
    if (Op == ISD::TRUNCATE) {
      const SDValue &TruncInput = Arg.getOperand(0);
      if (TruncInput.getOpcode() == ISD::AssertZext &&
          cast<VTSDNode>(TruncInput.getOperand(1))->getVT() ==
              Arg.getValueType()) {
        Arg = TruncInput.getOperand(0);
        continue;
      }
    }
    break;
  }

  int FI = INT_MAX;
  if (Arg.getOpcode() == ISD::CopyFromReg) {
    unsigned VR = cast<RegisterSDNode>(Arg.getOperand(1))->getReg();
    if (!TargetRegisterInfo::isVirtualRegister(VR))
      return false;
    MachineInstr *Def = MRI->getVRegDef(VR);
    if (!Def)
      return false;
    if (!Flags.isByVal()) {
      if (!TII->isLoadFromStackSlot(*Def, FI))
        return false;
    } else {
      unsigned Opcode = Def->getOpcode();
      if ((Opcode == M680x0::LEA32p || Opcode == M680x0::LEA32f) &&
          Def->getOperand(1).isFI()) {
        FI = Def->getOperand(1).getIndex();
        Bytes = Flags.getByValSize();
      } else
        return false;
    }
  } else if (LoadSDNode *Ld = dyn_cast<LoadSDNode>(Arg)) {
    if (Flags.isByVal())
      // ByVal argument is passed in as a pointer but it's now being
      // dereferenced. e.g.
      // define @foo(%struct.X* %A) {
      //   tail call @bar(%struct.X* byval %A)
      // }
      return false;
    SDValue Ptr = Ld->getBasePtr();
    FrameIndexSDNode *FINode = dyn_cast<FrameIndexSDNode>(Ptr);
    if (!FINode)
      return false;
    FI = FINode->getIndex();
  } else if (Arg.getOpcode() == ISD::FrameIndex && Flags.isByVal()) {
    FrameIndexSDNode *FINode = cast<FrameIndexSDNode>(Arg);
    FI = FINode->getIndex();
    Bytes = Flags.getByValSize();
  } else
    return false;

  assert(FI != INT_MAX);
  if (!MFI.isFixedObjectIndex(FI))
    return false;

  if (Offset != MFI.getObjectOffset(FI))
    return false;

  if (VA.getLocVT().getSizeInBits() > Arg.getValueType().getSizeInBits()) {
    // If the argument location is wider than the argument type, check that any
    // extension flags match.
    if (Flags.isZExt() != MFI.isObjectZExt(FI) ||
        Flags.isSExt() != MFI.isObjectSExt(FI)) {
      return false;
    }
  }

  return Bytes == MFI.getObjectSize(FI);
}

SDValue M680x0TargetLowering::
getReturnAddressFrameIndex(SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  M680x0MachineFunctionInfo *FuncInfo = MF.getInfo<M680x0MachineFunctionInfo>();
  int ReturnAddrIndex = FuncInfo->getRAIndex();

  if (ReturnAddrIndex == 0) {
    // Set up a frame object for the return address.
    unsigned SlotSize = Subtarget.getSlotSize();
    ReturnAddrIndex = MF.getFrameInfo()
      .CreateFixedObject(SlotSize, -(int64_t)SlotSize, false);
    FuncInfo->setRAIndex(ReturnAddrIndex);
  }

  return DAG.getFrameIndex(ReturnAddrIndex, getPointerTy(DAG.getDataLayout()));
}

SDValue M680x0TargetLowering::
EmitTailCallLoadRetAddr(SelectionDAG &DAG, SDValue &OutRetAddr, SDValue Chain,
                        bool IsTailCall, int FPDiff, const SDLoc &DL) const {
  EVT VT = getPointerTy(DAG.getDataLayout());
  OutRetAddr = getReturnAddressFrameIndex(DAG);

  // Load the "old" Return address.
  OutRetAddr = DAG.getLoad(VT, DL, Chain, OutRetAddr, MachinePointerInfo());
  return SDValue(OutRetAddr.getNode(), 1);
}

SDValue M680x0TargetLowering::
EmitTailCallStoreRetAddr(SelectionDAG &DAG, MachineFunction &MF,
                         SDValue Chain, SDValue RetFI, EVT PtrVT,
                         unsigned SlotSize, int FPDiff, const SDLoc &DL) const {
  if (!FPDiff) return Chain;

  // Calculate the new stack slot for the return address.
  int NewFO = MF.getFrameInfo()
    .CreateFixedObject(SlotSize, (int64_t)FPDiff - SlotSize, false);

  SDValue NewFI = DAG.getFrameIndex(NewFO, PtrVT);
  // Store the return address to the appropriate stack slot.
  Chain = DAG.getStore(Chain, DL, RetFI, NewFI,
                       MachinePointerInfo::getFixedStack(
                           DAG.getMachineFunction(), NewFO));
  return Chain;
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


SDValue M680x0TargetLowering::
LowerMemOpCallTo(SDValue Chain, SDValue StackPtr, SDValue Arg, const SDLoc &DL,
                 SelectionDAG &DAG, const CCValAssign &VA,
                 ISD::ArgFlagsTy Flags) const {
  unsigned LocMemOffset = VA.getLocMemOffset();
  SDValue PtrOff = DAG.getIntPtrConstant(LocMemOffset, DL);
  PtrOff = DAG.getNode(ISD::ADD, DL, getPointerTy(DAG.getDataLayout()),
                       StackPtr, PtrOff);
  if (Flags.isByVal())
    return CreateCopyOfByValArgument(Arg, PtrOff, Chain, Flags, DAG, DL);

  return DAG.getStore(
      Chain, DL, Arg, PtrOff,
      MachinePointerInfo::getStack(DAG.getMachineFunction(), LocMemOffset));
}

//===----------------------------------------------------------------------===//
//                                   Call
//===----------------------------------------------------------------------===//

SDValue M680x0TargetLowering::
LowerCall(TargetLowering::CallLoweringInfo &CLI,
          SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG                     = CLI.DAG;
  SDLoc &DL                             = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals     = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins   = CLI.Ins;
  SDValue Chain                         = CLI.Chain;
  SDValue Callee                        = CLI.Callee;
  CallingConv::ID CallConv              = CLI.CallConv;
  bool &isTailCall                      = CLI.IsTailCall;
  bool isVarArg                         = CLI.IsVarArg;

  MachineFunction &MF = DAG.getMachineFunction();
  StructReturnType SR = callIsStructReturn(Outs);
  bool IsSibcall      = false;
  M680x0MachineFunctionInfo *MFI = MF.getInfo<M680x0MachineFunctionInfo>();
  const M680x0RegisterInfo *TRI = Subtarget.getRegisterInfo();

  // TODO interrupts
  // if (CallConv == CallingConv::M680x0_INTR)
  //   report_fatal_error("M680x0 interrupts may not be called directly");

  auto Attr = MF.getFunction()->getFnAttribute("disable-tail-calls");
  if (Attr.getValueAsString() == "true")
    isTailCall = false;

  if (Subtarget.isPICStyleGOT() &&
      !MF.getTarget().Options.GuaranteedTailCallOpt) {
    // TODO reqd more about this stuff
    // If we are using a GOT, disable tail calls to external symbols with
    // default visibility. Tail calling such a symbol requires using a GOT
    // relocation, which forces early binding of the symbol. This breaks code
    // that require lazy function symbol resolution. Using musttail or
    // GuaranteedTailCallOpt will override this.
    GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee);
    if (!G || (!G->getGlobal()->hasLocalLinkage() &&
               G->getGlobal()->hasDefaultVisibility()))
      isTailCall = false;
  }

  bool IsMustTail = CLI.CS && CLI.CS->isMustTailCall();
  if (IsMustTail) {
    // Force this to be a tail call.  The verifier rules are enough to ensure
    // that we can lower this successfully without moving the return address
    // around.
    isTailCall = true;
  } else if (isTailCall) {
    // Check if it's really possible to do a tail call.
    isTailCall = IsEligibleForTailCallOptimization(Callee, CallConv,
                    isVarArg, SR != NotStructReturn,
                    MF.getFunction()->hasStructRetAttr(), CLI.RetTy,
                    Outs, OutVals, Ins, DAG);

    // Sibcalls are automatically detected tailcalls which do not require
    // ABI changes.
    if (!MF.getTarget().Options.GuaranteedTailCallOpt && isTailCall)
      IsSibcall = true;

    if (isTailCall)
      ++NumTailCalls;
  }

  assert(!(isVarArg && canGuaranteeTCO(CallConv)) &&
         "Var args not supported with calling convention fastcc");

  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeCallOperands(Outs, CC_M680x0);

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = CCInfo.getAlignedCallFrameSize();
  if (IsSibcall) {
    // This is a sibcall. The memory operands are available in caller's
    // own caller's stack.
    NumBytes = 0;
  } else if (MF.getTarget().Options.GuaranteedTailCallOpt &&
           canGuaranteeTCO(CallConv)) {
    NumBytes = GetAlignedArgumentStackSize(NumBytes, DAG);
  }

  // TODO debug this:
  int FPDiff = 0;
  if (isTailCall && !IsSibcall && !IsMustTail) {
    // Lower arguments at fp - stackoffset + fpdiff.
    unsigned NumBytesCallerPushed = MFI->getBytesToPopOnReturn();

    FPDiff = NumBytesCallerPushed - NumBytes;

    // Set the delta of movement of the returnaddr stackslot.
    // But only set if delta is greater than previous delta.
    if (FPDiff < MFI->getTCReturnAddrDelta())
      MFI->setTCReturnAddrDelta(FPDiff);
  }

  unsigned NumBytesToPush = NumBytes;
  unsigned NumBytesToPop = NumBytes;

  // If we have an inalloca argument, all stack space has already been allocated
  // for us and be right at the top of the stack.  We don't support multiple
  // arguments passed in memory when using inalloca.
  if (!Outs.empty() && Outs.back().Flags.isInAlloca()) {
    NumBytesToPush = 0;
    if (!ArgLocs.back().isMemLoc())
      report_fatal_error("cannot use inalloca attribute on a register "
                         "parameter");
    if (ArgLocs.back().getLocMemOffset() != 0)
      report_fatal_error("any parameter with the inalloca attribute must be "
                         "the only memory argument");
  }

  if (!IsSibcall)
    Chain = DAG.getCALLSEQ_START(
        Chain, DAG.getIntPtrConstant(NumBytesToPush, DL, true), DL);

  SDValue RetFI;
  // Load return address for tail calls.
  if (isTailCall && FPDiff)
    Chain = EmitTailCallLoadRetAddr(DAG, RetFI, Chain, isTailCall, FPDiff, DL);

  SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;
  SDValue StackPtr;

  // Walk the register/memloc assignments, inserting copies/loads.  In the case
  // of tail call optimization arguments are handle later.
  const M680x0RegisterInfo *RegInfo = Subtarget.getRegisterInfo();
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    ISD::ArgFlagsTy Flags = Outs[i].Flags;

    // Skip inalloca arguments, they have already been written.
    if (Flags.isInAlloca())
      continue;

    CCValAssign &VA = ArgLocs[i];
    EVT RegVT = VA.getLocVT();
    SDValue Arg = OutVals[i];
    bool isByVal = Flags.isByVal();

    // Promote the value if needed.
    switch (VA.getLocInfo()) {
    default: llvm_unreachable("Unknown loc info!");
    case CCValAssign::Full: break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, DL, RegVT, Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, DL, RegVT, Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, DL, RegVT, Arg);
      break;
    case CCValAssign::BCvt:
      Arg = DAG.getBitcast(RegVT, Arg);
      break;
    case CCValAssign::Indirect: {
      // Store the argument.
      SDValue SpillSlot = DAG.CreateStackTemporary(VA.getValVT());
      int FI = cast<FrameIndexSDNode>(SpillSlot)->getIndex();
      Chain = DAG.getStore(
          Chain, DL, Arg, SpillSlot,
          MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI));
      Arg = SpillSlot;
      break;
    }
    }

    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
    } else if (!IsSibcall && (!isTailCall || isByVal)) {
      assert(VA.isMemLoc());
      if (!StackPtr.getNode()) {
        StackPtr = DAG.getCopyFromReg(Chain, DL, RegInfo->getStackRegister(),
                                      getPointerTy(DAG.getDataLayout()));
      }
      MemOpChains.push_back(LowerMemOpCallTo(Chain, StackPtr, Arg,
                                             DL, DAG, VA, Flags));
    }
  }

  if (!MemOpChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);

  if (Subtarget.isPICStyleGOT()) {
    // ELF / PIC requires GOT in the %BP register before function calls via PLT
    // GOT pointer.
    if (!isTailCall) {
      RegsToPass.push_back(std::make_pair(
          unsigned(TRI->getBaseRegister()),
            DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(),
            getPointerTy(DAG.getDataLayout()))));
    } else {
      // ??? WUT, debug this
      // If we are tail calling and generating PIC/GOT style code load the
      // address of the callee into %A1. The value in %A1 is used as target of
      // the tail jump. This is done to circumvent the %BP/callee-saved problem
      // for tail calls on PIC/GOT architectures. Normally we would just put the
      // address of GOT into %BP and then call target@PLT. But for tail calls
      // %BP would be restored (since %BP is callee saved) before jumping to the
      // target@PLT.

      // NOTE: The actual moving to %A1 is done further down.
      GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee);
      if (G && !G->getGlobal()->hasLocalLinkage() &&
          G->getGlobal()->hasDefaultVisibility())
        Callee = LowerGlobalAddress(Callee, DAG);
      else if (isa<ExternalSymbolSDNode>(Callee))
        Callee = LowerExternalSymbol(Callee, DAG);
    }
  }

  if (isVarArg && IsMustTail) {
    const auto &Forwards = MFI->getForwardedMustTailRegParms();
    for (const auto &F : Forwards) {
      SDValue Val = DAG.getCopyFromReg(Chain, DL, F.VReg, F.VT);
      RegsToPass.push_back(std::make_pair(unsigned(F.PReg), Val));
    }
  }

  // For tail calls lower the arguments to the 'real' stack slots.  Sibcalls
  // don't need this because the eligibility check rejects calls that require
  // shuffling arguments passed in memory.
  if (!IsSibcall && isTailCall) {
    // Force all the incoming stack arguments to be loaded from the stack
    // before any new outgoing arguments are stored to the stack, because the
    // outgoing stack slots may alias the incoming argument stack slots, and
    // the alias isn't otherwise explicit. This is slightly more conservative
    // than necessary, because it means that each store effectively depends
    // on every argument instead of just those arguments it would clobber.
    SDValue ArgChain = DAG.getStackArgumentTokenFactor(Chain);

    SmallVector<SDValue, 8> MemOpChains2;
    SDValue FIN;
    int FI = 0;
    for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
      CCValAssign &VA = ArgLocs[i];
      if (VA.isRegLoc())
        continue;
      assert(VA.isMemLoc());
      SDValue Arg = OutVals[i];
      ISD::ArgFlagsTy Flags = Outs[i].Flags;
      // Skip inalloca arguments.  They don't require any work.
      if (Flags.isInAlloca())
        continue;
      // Create frame index.
      int32_t Offset = VA.getLocMemOffset()+FPDiff;
      uint32_t OpSize = (VA.getLocVT().getSizeInBits()+7)/8;
      FI = MF.getFrameInfo().CreateFixedObject(OpSize, Offset, true);
      FIN = DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));

      if (Flags.isByVal()) {
        // Copy relative to framepointer.
        SDValue Source = DAG.getIntPtrConstant(VA.getLocMemOffset(), DL);
        if (!StackPtr.getNode()) {
          StackPtr = DAG.getCopyFromReg(Chain, DL, RegInfo->getStackRegister(),
                                        getPointerTy(DAG.getDataLayout()));
        }
        Source = DAG.getNode(ISD::ADD, DL, getPointerTy(DAG.getDataLayout()),
                             StackPtr, Source);

        MemOpChains2.push_back(
            CreateCopyOfByValArgument(Source, FIN, ArgChain, Flags, DAG, DL));
      } else {
        // Store relative to framepointer.
        MemOpChains2.push_back(DAG.getStore(ArgChain, DL, Arg, FIN,
            MachinePointerInfo::getFixedStack(DAG.getMachineFunction(), FI)));
      }
    }

    if (!MemOpChains2.empty())
      Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains2);

    // Store the return address to the appropriate stack slot.
    Chain = EmitTailCallStoreRetAddr(DAG, MF, Chain, RetFI,
                                     getPointerTy(DAG.getDataLayout()),
                                     Subtarget.getSlotSize(), FPDiff, DL);
  }

  // Build a sequence of copy-to-reg nodes chained together with token chain
  // and flag operands which copy the outgoing args into registers.
  SDValue InFlag;
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Chain = DAG.getCopyToReg(Chain, DL, RegsToPass[i].first,
                             RegsToPass[i].second, InFlag);
    InFlag = Chain.getValue(1);
  }

  if (Callee->getOpcode() == ISD::GlobalAddress) {
    // If the callee is a GlobalAddress node (quite common, every direct call
    // is) turn it into a TargetGlobalAddress node so that legalize doesn't hack
    // it.
    GlobalAddressSDNode* G = cast<GlobalAddressSDNode>(Callee);

    // We should use extra load for direct calls to dllimported functions in
    // non-JIT mode.
    const GlobalValue *GV = G->getGlobal();
    if (!GV->hasDLLImportStorageClass()) {
      unsigned char OpFlags = Subtarget.classifyGlobalFunctionReference(GV);

      Callee = DAG.getTargetGlobalAddress(
          GV, DL, getPointerTy(DAG.getDataLayout()), G->getOffset(), OpFlags);

      if (OpFlags == M680x0II::MO_GOTPCREL) {

        // Add a wrapper.
        Callee = DAG.getNode(M680x0ISD::WrapperPC, DL,
          getPointerTy(DAG.getDataLayout()), Callee);

        // Add extra indirection
        Callee = DAG.getLoad(
            getPointerTy(DAG.getDataLayout()), DL, DAG.getEntryNode(), Callee,
            MachinePointerInfo::getGOT(DAG.getMachineFunction()));
      }
    }
  } else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(Callee)) {
    const Module *Mod = DAG.getMachineFunction().getFunction()->getParent();
    unsigned char OpFlags =
        Subtarget.classifyGlobalFunctionReference(nullptr, *Mod);

    Callee = DAG.getTargetExternalSymbol(
        S->getSymbol(), getPointerTy(DAG.getDataLayout()), OpFlags);
  }

  // Returns a chain & a flag for retval copy to use.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  SmallVector<SDValue, 8> Ops;

  if (!IsSibcall && isTailCall) {
    Chain = DAG.getCALLSEQ_END(Chain,
                               DAG.getIntPtrConstant(NumBytesToPop, DL, true),
                               DAG.getIntPtrConstant(0, DL, true), InFlag, DL);
    InFlag = Chain.getValue(1);
  }

  Ops.push_back(Chain);
  Ops.push_back(Callee);

  if (isTailCall)
    Ops.push_back(DAG.getConstant(FPDiff, DL, MVT::i32));

  // Add argument registers to the end of the list so that they are known live
  // into the call.
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i)
    Ops.push_back(DAG.getRegister(RegsToPass[i].first,
                                  RegsToPass[i].second.getValueType()));

  // Add a register mask operand representing the call-preserved registers.
  const uint32_t *Mask = RegInfo->getCallPreservedMask(MF, CallConv);
  assert(Mask && "Missing call preserved mask for calling convention");

  Ops.push_back(DAG.getRegisterMask(Mask));

  if (InFlag.getNode())
    Ops.push_back(InFlag);

  if (isTailCall) {
    MF.getFrameInfo().setHasTailCall();
    return DAG.getNode(M680x0ISD::TC_RETURN, DL, NodeTys, Ops);
  }

  Chain = DAG.getNode(M680x0ISD::CALL, DL, NodeTys, Ops);
  InFlag = Chain.getValue(1);

  // Create the CALLSEQ_END node.
  unsigned NumBytesForCalleeToPop;
  if (M680x0::isCalleePop(CallConv, isVarArg,
                       DAG.getTarget().Options.GuaranteedTailCallOpt)) {
    NumBytesForCalleeToPop = NumBytes;    // Callee pops everything
  } else if (!canGuaranteeTCO(CallConv) && SR == StackStructReturn) {
    // If this is a call to a struct-return function, the callee
    // pops the hidden struct pointer, so we have to push it back.
    NumBytesForCalleeToPop = 4;
  } else {
    NumBytesForCalleeToPop = 0;  // Callee pops nothing.
  }

  if (CLI.DoesNotReturn && !getTargetMachine().Options.TrapUnreachable) {
    // No need to reset the stack after the call if the call doesn't return. To
    // make the MI verify, we'll pretend the callee does it for us.
    NumBytesForCalleeToPop = NumBytes;
  }

  // Returns a flag for retval copy to use.
  if (!IsSibcall) {
    Chain = DAG.getCALLSEQ_END(Chain,
                               DAG.getIntPtrConstant(NumBytesToPop, DL, true),
                               DAG.getIntPtrConstant(NumBytesForCalleeToPop, DL,
                                                     true),
                               InFlag, DL);
    InFlag = Chain.getValue(1);
  }

  // Handle result values, copying them out of physregs into vregs that we
  // return.
  return LowerCallResult(Chain, InFlag, CallConv, isVarArg,
                         Ins, DL, DAG, InVals);
}

SDValue M680x0TargetLowering::
LowerCallResult(SDValue Chain, SDValue InFlag, CallingConv::ID CallConv,
                bool isVarArg, const SmallVectorImpl<ISD::InputArg> &Ins,
                const SDLoc &DL, SelectionDAG &DAG,
                SmallVectorImpl<SDValue> &InVals) const {

  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeCallResult(Ins, RetCC_M680x0);

  // Copy all of the result registers out of their specified physreg.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i) {
    CCValAssign &VA = RVLocs[i];
    EVT CopyVT = VA.getLocVT();

    /// ??? is this correct?
    Chain = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), CopyVT, InFlag)
               .getValue(1);
    SDValue Val = Chain.getValue(0);

    if (VA.isExtInLoc() && VA.getValVT().getScalarType() == MVT::i1)
      Val = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Val);

    InFlag = Chain.getValue(2);
    InVals.push_back(Val);
  }

  return Chain;
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
   if (MFI.hasVAStart()) {
     MMFI->setVarArgsFrameIndex(MFI.CreateFixedObject(1, StackSize, true));
   }

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

//===----------------------------------------------------------------------===//
//                Fast Calling Convention (tail call) implementation
//===----------------------------------------------------------------------===//

//  Like std call, callee cleans arguments, convention except that ECX is
//  reserved for storing the tail called function address. Only 2 registers are
//  free for argument passing (inreg). Tail call optimization is performed
//  provided:
//                * tailcallopt is enabled
//                * caller/callee are fastcc
//  On M680x0_64 architecture with GOT-style position independent code only local
//  (within module) calls are supported at the moment.
//  To keep the stack aligned according to platform abi the function
//  GetAlignedArgumentStackSize ensures that argument delta is always multiples
//  of stack alignment. (Dynamic linkers need this - darwin's dyld for example)
//  If a tail called function callee has more arguments than the caller the
//  caller needs to make sure that there is room to move the RETADDR to. This is
//  achieved by reserving an area the size of the argument delta right after the
//  original RETADDR, but before the saved framepointer or the spilled registers
//  e.g. caller(arg1, arg2) calls callee(arg1, arg2,arg3,arg4)
//  stack layout:
//    arg1
//    arg2
//    RETADDR
//    [ new RETADDR
//      move area ]
//    (possible EBP)
//    ESI
//    EDI
//    local1 ..

/// Make the stack size align e.g 16n + 12 aligned for a 16-byte align
/// requirement.
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

/// Check whether the call is eligible for tail call optimization. Targets
/// that want to do tail call optimization should implement this function.
bool M680x0TargetLowering::
IsEligibleForTailCallOptimization(SDValue Callee, CallingConv::ID CalleeCC,
    bool isVarArg, bool isCalleeStructRet, bool isCallerStructRet,
    Type *RetTy, const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals,
    const SmallVectorImpl<ISD::InputArg> &Ins, SelectionDAG &DAG) const {
  if (!mayTailCallThisCC(CalleeCC))
    return false;

  // If -tailcallopt is specified, make fastcc functions tail-callable.
  MachineFunction &MF = DAG.getMachineFunction();
  const Function *CallerF = MF.getFunction();

  CallingConv::ID CallerCC = CallerF->getCallingConv();
  bool CCMatch = CallerCC == CalleeCC;

  if (DAG.getTarget().Options.GuaranteedTailCallOpt) {
    if (canGuaranteeTCO(CalleeCC) && CCMatch)
      return true;
    return false;
  }

  // Look for obvious safe cases to perform tail call optimization that do not
  // require ABI changes. This is what gcc calls sibcall.

  // Can't do sibcall if stack needs to be dynamically re-aligned. PEI needs to
  // emit a special epilogue.
  const M680x0RegisterInfo *RegInfo = Subtarget.getRegisterInfo();
  if (RegInfo->needsStackRealignment(MF))
    return false;

  // Also avoid sibcall optimization if either caller or callee uses struct
  // return semantics.
  if (isCalleeStructRet || isCallerStructRet)
    return false;

  // Do not sibcall optimize vararg calls unless all arguments are passed via
  // registers.
  LLVMContext &C = *DAG.getContext();
  if (isVarArg && !Outs.empty()) {

    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CalleeCC, isVarArg, MF, ArgLocs, C);

    CCInfo.AnalyzeCallOperands(Outs, CC_M680x0);
    for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i)
      if (!ArgLocs[i].isRegLoc())
        return false;
  }

  // Check that the call results are passed in the same way.
  if (!CCState::resultsCompatible(CalleeCC, CallerCC, MF, C, Ins,
                                  RetCC_M680x0, RetCC_M680x0))
    return false;

  // The callee has to preserve all registers the caller needs to preserve.
  const M680x0RegisterInfo *TRI = Subtarget.getRegisterInfo();
  const uint32_t *CallerPreserved = TRI->getCallPreservedMask(MF, CallerCC);
  if (!CCMatch) {
    const uint32_t *CalleePreserved = TRI->getCallPreservedMask(MF, CalleeCC);
    if (!TRI->regmaskSubsetEqual(CallerPreserved, CalleePreserved))
      return false;
  }

  unsigned StackArgsSize = 0;

  // If the callee takes no arguments then go on to check the results of the
  // call.
  if (!Outs.empty()) {
    // Check if stack adjustment is needed. For now, do not do this if any
    // argument is passed on the stack.
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CalleeCC, isVarArg, MF, ArgLocs, C);

    CCInfo.AnalyzeCallOperands(Outs, CC_M680x0);
    StackArgsSize = CCInfo.getNextStackOffset();

    if (CCInfo.getNextStackOffset()) {
      // Check if the arguments are already laid out in the right way as
      // the caller's fixed stack objects.
      MachineFrameInfo &MFI = MF.getFrameInfo();
      const MachineRegisterInfo *MRI = &MF.getRegInfo();
      const M680x0InstrInfo *TII = Subtarget.getInstrInfo();
      for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
        CCValAssign &VA = ArgLocs[i];
        SDValue Arg = OutVals[i];
        ISD::ArgFlagsTy Flags = Outs[i].Flags;
        if (VA.getLocInfo() == CCValAssign::Indirect)
          return false;
        if (!VA.isRegLoc()) {
          if (!MatchingStackOffset(Arg, VA.getLocMemOffset(), Flags,
                                   MFI, MRI, TII, VA))
            return false;
        }
      }
    }

    bool PositionIndependent = isPositionIndependent();
    // If the tailcall address may be in a register, then make sure it's
    // possible to register allocate for it. The call address can
    // only target %A0 or %A1 since the tail call must be scheduled after
    // callee-saved registers are restored. These happen to be the same
    // registers used to pass 'inreg' arguments so watch out for those.
    if ((!isa<GlobalAddressSDNode>(Callee) && !isa<ExternalSymbolSDNode>(Callee))
        || PositionIndependent) {
      unsigned NumInRegs = 0;
      // In PIC we need an extra register to formulate the address computation
      // for the callee.
      unsigned MaxInRegs = PositionIndependent ? 1 : 2;

      for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
        CCValAssign &VA = ArgLocs[i];
        if (!VA.isRegLoc())
          continue;
        unsigned Reg = VA.getLocReg();
        switch (Reg) {
        default: break;
        case M680x0::A0: case M680x0::A1:
          if (++NumInRegs == MaxInRegs)
            return false;
          break;
        }
      }
    }

    const MachineRegisterInfo &MRI = MF.getRegInfo();
    if (!parametersInCSRMatch(MRI, CallerPreserved, ArgLocs, OutVals))
      return false;
  }

  bool CalleeWillPop =
      M680x0::isCalleePop(CalleeCC, isVarArg,
                       MF.getTarget().Options.GuaranteedTailCallOpt);

  if (unsigned BytesToPop =
          MF.getInfo<M680x0MachineFunctionInfo>()->getBytesToPopOnReturn()) {
    // If we have bytes to pop, the callee must pop them.
    bool CalleePopMatches = CalleeWillPop && BytesToPop == StackArgsSize;
    if (!CalleePopMatches)
      return false;
  } else if (CalleeWillPop && StackArgsSize > 0) {
    // If we don't have bytes to pop, make sure the callee doesn't pop any.
    return false;
  }

  return true;
}

// ConstantPool, JumpTable, GlobalAddress, and ExternalSymbol are lowered as
// their target countpart wrapped in the M680x0ISD::Wrapper node. Suppose N is
// one of the above mentioned nodes. It has to be wrapped because otherwise
// Select(N) returns N. So the raw TargetGlobalAddress nodes, etc. can only
// be used to form addressing mode. These wrapped nodes will be selected
// into MOV32ri.
SDValue M680x0TargetLowering::
LowerConstantPool(SDValue Op, SelectionDAG &DAG) const {
  ConstantPoolSDNode *CP = cast<ConstantPoolSDNode>(Op);

  // In PIC mode (unless we're in RIPRel PIC mode) we add an offset to the
  // global base reg.
  unsigned char OpFlag = Subtarget.classifyLocalReference(nullptr);
  unsigned WrapperKind = M680x0ISD::Wrapper;
  CodeModel::Model M = DAG.getTarget().getCodeModel();

  if (Subtarget.isPICStylePCRel() &&
      (M == CodeModel::Small || M == CodeModel::Kernel))
    WrapperKind = M680x0ISD::WrapperPC;

  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result = DAG.getTargetConstantPool(
      CP->getConstVal(), PtrVT, CP->getAlignment(), CP->getOffset(), OpFlag);
  SDLoc DL(CP);
  Result = DAG.getNode(WrapperKind, DL, PtrVT, Result);
  // With PIC, the address is actually $g + Offset.
  if (OpFlag) {
    Result =
        DAG.getNode(ISD::ADD, DL, PtrVT,
                    DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(), PtrVT), Result);
  }

  return Result;
}

SDValue M680x0TargetLowering::
LowerJumpTable(SDValue Op, SelectionDAG &DAG) const {
  JumpTableSDNode *JT = cast<JumpTableSDNode>(Op);

  // In PIC mode (unless we're in RIPRel PIC mode) we add an offset to the
  // global base reg.
  unsigned char OpFlag = Subtarget.classifyLocalReference(nullptr);
  unsigned WrapperKind = M680x0ISD::Wrapper;
  CodeModel::Model M = DAG.getTarget().getCodeModel();

  if (Subtarget.isPICStylePCRel() &&
      (M == CodeModel::Small || M == CodeModel::Kernel))
    WrapperKind = M680x0ISD::WrapperPC;

  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result = DAG.getTargetJumpTable(JT->getIndex(), PtrVT, OpFlag);
  SDLoc DL(JT);
  Result = DAG.getNode(WrapperKind, DL, PtrVT, Result);

  // With PIC, the address is actually $g + Offset.
  if (OpFlag)
    Result =
        DAG.getNode(ISD::ADD, DL, PtrVT,
                    DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(), PtrVT), Result);

  return Result;
}

SDValue M680x0TargetLowering::
LowerExternalSymbol(SDValue Op, SelectionDAG &DAG) const {
  const char *Sym = cast<ExternalSymbolSDNode>(Op)->getSymbol();

  // In PIC mode (unless we're in RIPRel PIC mode) we add an offset to the
  // global base reg.
  const Module *Mod = DAG.getMachineFunction().getFunction()->getParent();
  unsigned char OpFlag = Subtarget.classifyGlobalReference(nullptr, *Mod);
  unsigned WrapperKind = M680x0ISD::Wrapper;
  CodeModel::Model M = DAG.getTarget().getCodeModel();

  if (Subtarget.isPICStylePCRel() &&
      (M == CodeModel::Small || M == CodeModel::Kernel))
    WrapperKind = M680x0ISD::WrapperPC;

  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result = DAG.getTargetExternalSymbol(Sym, PtrVT, OpFlag);

  SDLoc DL(Op);
  Result = DAG.getNode(WrapperKind, DL, PtrVT, Result);

  // With PIC, the address is actually $g + Offset.
  if (isPositionIndependent()) {
    Result =
        DAG.getNode(ISD::ADD, DL, PtrVT,
                    DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(), PtrVT), Result);
  }

  // For symbols that require a load from a stub to get the address, emit the
  // load.
  if (isGlobalStubReference(OpFlag))
    Result = DAG.getLoad(PtrVT, DL, DAG.getEntryNode(), Result,
                         MachinePointerInfo::getGOT(DAG.getMachineFunction()));

  return Result;
}

SDValue M680x0TargetLowering::
LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const {
  // Create the TargetBlockAddressAddress node.
  unsigned char OpFlags =
    Subtarget.classifyBlockAddressReference();
  CodeModel::Model M = DAG.getTarget().getCodeModel();
  const BlockAddress *BA = cast<BlockAddressSDNode>(Op)->getBlockAddress();
  int64_t Offset = cast<BlockAddressSDNode>(Op)->getOffset();
  SDLoc DL(Op);
  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result = DAG.getTargetBlockAddress(BA, PtrVT, Offset, OpFlags);

  if (Subtarget.isPICStylePCRel() &&
      (M == CodeModel::Small || M == CodeModel::Kernel))
    Result = DAG.getNode(M680x0ISD::WrapperPC, DL, PtrVT, Result);
  else
    Result = DAG.getNode(M680x0ISD::Wrapper, DL, PtrVT, Result);

  // With PIC, the address is actually $g + Offset.
  if (isGlobalRelativeToPICBase(OpFlags)) {
    Result = DAG.getNode(ISD::ADD, DL, PtrVT,
                         DAG.getNode(M680x0ISD::GlobalBaseReg, DL, PtrVT), Result);
  }

  return Result;
}

SDValue M680x0TargetLowering::
LowerGlobalAddress(const GlobalValue *GV, const SDLoc &DL, int64_t Offset,
                   SelectionDAG &DAG) const {
  // Create the TargetGlobalAddress node, folding in the constant
  // offset if it is legal.
  unsigned char OpFlags = Subtarget.classifyGlobalReference(GV);
  CodeModel::Model M = DAG.getTarget().getCodeModel();
  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result;
  if (OpFlags == M680x0II::MO_NO_FLAG &&
      M680x0::isOffsetSuitableForCodeModel(Offset, M)) {
    // A direct static reference to a global.
    Result = DAG.getTargetGlobalAddress(GV, DL, PtrVT, Offset);
    Offset = 0;
  } else {
    Result = DAG.getTargetGlobalAddress(GV, DL, PtrVT, 0, OpFlags);
  }

  if (Subtarget.isPICStylePCRel() &&
      (M == CodeModel::Small || M == CodeModel::Kernel))
    Result = DAG.getNode(M680x0ISD::WrapperPC, DL, PtrVT, Result);
  else
    Result = DAG.getNode(M680x0ISD::Wrapper, DL, PtrVT, Result);

  // With PIC, the address is actually $g + Offset.
  if (isGlobalRelativeToPICBase(OpFlags)) {
    Result = DAG.getNode(ISD::ADD, DL, PtrVT,
                         DAG.getNode(M680x0ISD::GlobalBaseReg, DL, PtrVT), Result);
  }

  // For globals that require a load from a stub to get the address, emit the
  // load.
  if (isGlobalStubReference(OpFlags))
    Result = DAG.getLoad(PtrVT, DL, DAG.getEntryNode(), Result,
                         MachinePointerInfo::getGOT(DAG.getMachineFunction()));

  // If there was a non-zero offset that we didn't fold, create an explicit
  // addition for it.
  if (Offset != 0)
    Result = DAG.getNode(ISD::ADD, DL, PtrVT, Result,
                         DAG.getConstant(Offset, DL, PtrVT));

  return Result;
}

SDValue M680x0TargetLowering::
LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const {
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();
  return LowerGlobalAddress(GV, SDLoc(Op), Offset, DAG);
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

bool M680x0::
isOffsetSuitableForCodeModel(int64_t Offset, CodeModel::Model M,
                             bool hasSymbolicDisplacement) {
  // Offset should fit into 16 bit immediate field.
  // TODO Once x20 ISA is available change this
  if (!isInt<16>(Offset))
    return false;

  // If we don't have a symbolic displacement - we don't have any extra
  // restrictions.
  if (!hasSymbolicDisplacement)
    return true;

  // FIXME: Some tweaks might be needed for medium code model.
  if (M != CodeModel::Small && M != CodeModel::Kernel)
    return false;

  // For small code model we assume that latest object is 16MB before end of 31
  // bits boundary. We may also accept pretty large negative constants knowing
  // that all objects are in the positive half of address space.
  // ??? 16MB? is that right?
  // if (M == CodeModel::Small && Offset < 16*1024*1024)
  //   return true;

  // For kernel code model we know that all object resist in the negative half
  // of 32bits address space. We may not accept negative offsets, since they may
  // be just off and we may accept pretty large positive ones.
  // if (M == CodeModel::Kernel && Offset >= 0)
  //   return true;

  return false;
}

/// Determines whether the callee is required to pop its own arguments.
/// Callee pop is necessary to support tail calls.
bool M680x0::
isCalleePop(CallingConv::ID CallingConv, bool IsVarArg, bool GuaranteeTCO) {
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
