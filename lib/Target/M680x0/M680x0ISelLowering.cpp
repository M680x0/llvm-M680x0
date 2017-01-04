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
#include "llvm/CodeGen/MachineJumpTableInfo.h"
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

M680x0TargetLowering::
M680x0TargetLowering(const M680x0TargetMachine &TM, const M680x0Subtarget &STI)
    : TargetLowering(TM), Subtarget(STI), TM(TM) {

  MVT PtrVT = MVT::i32;

  setBooleanContents(ZeroOrOneBooleanContent);

  auto *RegInfo = Subtarget.getRegisterInfo();
  setStackPointerRegisterToSaveRestore(RegInfo->getStackRegister());

  ValueTypeActions.setTypeAction(MVT::i64, TypeExpandInteger);

  // Use _setjmp/_longjmp instead of setjmp/longjmp.
  setUseUnderscoreSetJmp(true);
  setUseUnderscoreLongJmp(true);

  // NOTE The stuff that follows is true for M68000

  // Set up the register classes.
  addRegisterClass(MVT::i8,  &M680x0::DR8RegClass);
  addRegisterClass(MVT::i16, &M680x0::XR16RegClass);
  addRegisterClass(MVT::i32, &M680x0::XR32RegClass);

  for (auto VT : MVT::integer_valuetypes())
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i1, Promote);

  // We don't accept any truncstore of integer registers.
  setTruncStoreAction(MVT::i64, MVT::i32, Expand);
  setTruncStoreAction(MVT::i64, MVT::i16, Expand);
  setTruncStoreAction(MVT::i64, MVT::i8 , Expand);
  setTruncStoreAction(MVT::i32, MVT::i16, Expand);
  setTruncStoreAction(MVT::i32, MVT::i8 , Expand);
  setTruncStoreAction(MVT::i16, MVT::i8,  Expand);

  setOperationAction(ISD::MUL, MVT::i8,  Legal);
  setOperationAction(ISD::MUL, MVT::i16, Legal);
  setOperationAction(ISD::MUL, MVT::i32, Custom);

  for (auto OP : { ISD::SDIV, ISD::UDIV,
                   ISD::SREM, ISD::UREM,
                   ISD::MULHS, ISD::MULHU,
                   ISD::UMUL_LOHI, ISD::SMUL_LOHI }) {
    setOperationAction(OP, MVT::i8,  Legal);
    setOperationAction(OP, MVT::i16, Legal);
    setOperationAction(OP, MVT::i32, LibCall);
  }

  for (auto OP : { ISD::UMUL_LOHI, ISD::SMUL_LOHI }) {
    setOperationAction(OP, MVT::i8,  Expand);
    setOperationAction(OP, MVT::i16, Expand);
  }

  for (auto OP : { ISD::SMULO, ISD::UMULO }) {
    setOperationAction(OP, MVT::i8,  Expand);
    setOperationAction(OP, MVT::i16, Expand); //FIXME something wrong with custom lowering here
    setOperationAction(OP, MVT::i32, Expand);
  }

  // Add/Sub overflow ops with MVT::Glues are lowered to CCR dependences.
  for (auto VT : { MVT::i8, MVT::i16, MVT::i32 }) {
    setOperationAction(ISD::ADDC,  VT, Custom);
    setOperationAction(ISD::ADDE,  VT, Custom);
    setOperationAction(ISD::SUBC,  VT, Custom);
    setOperationAction(ISD::SUBE,  VT, Custom);
  }

  // SADDO and friends are legal with this setup, i hope
  for (auto VT : { MVT::i8, MVT::i16, MVT::i32 }) {
    setOperationAction(ISD::SADDO, VT, Custom);
    setOperationAction(ISD::UADDO, VT, Custom);
    setOperationAction(ISD::SSUBO, VT, Custom);
    setOperationAction(ISD::USUBO, VT, Custom);
  }

  setOperationAction(ISD::BR_JT,  MVT::Other, Expand);
  setOperationAction(ISD::BRCOND, MVT::Other, Custom);

  for (auto VT : { MVT::i8, MVT::i16, MVT::i32 }) {
    setOperationAction(ISD::BR_CC,     VT, Expand);
    setOperationAction(ISD::SELECT,    VT, Custom);
    setOperationAction(ISD::SELECT_CC, VT, Expand);
    setOperationAction(ISD::SETCC,     VT, Custom);
    setOperationAction(ISD::SETCCE,    VT, Custom);
  }

  setOperationAction(ISD::ConstantPool    , MVT::i32, Custom);
  setOperationAction(ISD::JumpTable       , MVT::i32, Custom);
  setOperationAction(ISD::GlobalAddress   , MVT::i32, Custom);
  setOperationAction(ISD::GlobalTLSAddress, MVT::i32, Custom);
  setOperationAction(ISD::ExternalSymbol  , MVT::i32, Custom);
  setOperationAction(ISD::BlockAddress    , MVT::i32, Custom);

  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAEND,   MVT::Other, Expand);
  setOperationAction(ISD::VAARG,   MVT::Other, Expand);
  setOperationAction(ISD::VACOPY,  MVT::Other, Expand);

  setOperationAction(ISD::DYNAMIC_STACKALLOC, PtrVT, Custom);

  computeRegisterProperties(STI.getRegisterInfo());

  setMinFunctionAlignment(2); // 2^2 bytes // ??? can it be just 2^1?

}

EVT M680x0TargetLowering::
getSetCCResultType(const DataLayout &DL, LLVMContext& Context, EVT VT) const {
  // M680x0 SETcc producess either 0x00 or 0xFF
  return MVT::i8;
}

MVT M680x0TargetLowering::
getScalarShiftAmountTy(const DataLayout &DL, EVT Ty) const {
  if (Ty.isSimple()) {
    return Ty.getSimpleVT();
  }
  return MVT::getIntegerVT(8 * DL.getPointerSize(0));
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

  // Because we are dealing with BE architecture we need to offset loading of
  // partial types
  int Offset = VA.getLocMemOffset();
  if (VA.getValVT() == MVT::i8) {
    Offset += 3;
  } else if (VA.getValVT() == MVT::i16) {
    Offset += 2;
  }

  // Calculate SP offset of interrupt parameter, re-arrange the slot normally
  // taken by a return address.
  // TODO interrupts
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
    int FI = MFI.CreateFixedObject(Bytes, Offset, isImmutable);
    // Adjust SP offset of interrupt parameter.
    // TODO interrupts
    // if (CallConv == CallingConv::M680x0_INTR) {
    //   MFI.setObjectOffset(FI, Offset);
    // }
    return DAG.getFrameIndex(FI, getPointerTy(DAG.getDataLayout()));
  } else {
    int FI = MFI.CreateFixedObject(ValVT.getSizeInBits()/8, Offset, isImmutable);

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
  // const M680x0RegisterInfo *TRI = Subtarget.getRegisterInfo();

  // TODO interrupts
  // if (CallConv == CallingConv::M680x0_INTR)
  //   report_fatal_error("M680x0 interrupts may not be called directly");

  auto Attr = MF.getFunction()->getFnAttribute("disable-tail-calls");
  if (Attr.getValueAsString() == "true")
    isTailCall = false;

  // FIXME check this
  // if (Subtarget.isPICStyleGOT() &&
  //     !MF.getTarget().Options.GuaranteedTailCallOpt) {
  //   // TODO reqd more about this stuff
  //   // If we are using a GOT, disable tail calls to external symbols with
  //   // default visibility. Tail calling such a symbol requires using a GOT
  //   // relocation, which forces early binding of the symbol. This breaks code
  //   // that require lazy function symbol resolution. Using musttail or
  //   // GuaranteedTailCallOpt will override this.
  //   GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee);
  //   if (!G || (!G->getGlobal()->hasLocalLinkage() &&
  //              G->getGlobal()->hasDefaultVisibility()))
  //     isTailCall = false;
  // }

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
  // It is empty for LibCall
  const Function *CalleeFunc = CLI.CS ? CLI.CS->getCalledFunction() : nullptr;
  M680x0CCState CCInfo(CalleeFunc, CallConv,
                       isVarArg, MF, ArgLocs, *DAG.getContext());
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

  // FIXME check this
  // ??? The only time GOT is really needed is for Medium-PIC static data
  // ??? otherwise we are happy with pc-rel or static references
  // if (Subtarget.isPICStyleGOT()) {
  //   // ELF / PIC requires GOT in the %BP register before function calls via PLT
  //   // GOT pointer.
  //   if (!isTailCall) {
  //     RegsToPass.push_back(std::make_pair(
  //         unsigned(TRI->getBaseRegister()),
  //           DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(),
  //           getPointerTy(DAG.getDataLayout()))));
  //   } else {
  //     // ??? WUT, debug this
  //     // If we are tail calling and generating PIC/GOT style code load the
  //     // address of the callee into %A1. The value in %A1 is used as target of
  //     // the tail jump. This is done to circumvent the %BP/callee-saved problem
  //     // for tail calls on PIC/GOT architectures. Normally we would just put the
  //     // address of GOT into %BP and then call target@PLT. But for tail calls
  //     // %BP would be restored (since %BP is callee saved) before jumping to the
  //     // target@PLT.
  //
  //     // NOTE: The actual moving to %A1 is done further down.
  //     GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee);
  //     if (G && !G->getGlobal()->hasLocalLinkage() &&
  //         G->getGlobal()->hasDefaultVisibility())
  //       Callee = LowerGlobalAddress(Callee, DAG);
  //     else if (isa<ExternalSymbolSDNode>(Callee))
  //       Callee = LowerExternalSymbol(Callee, DAG);
  //   }
  // }

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
  M680x0CCState CCInfo(MF.getFunction(), CCID, isVarArg,
                       MF, ArgLocs, *DAG.getContext());

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

//===----------------------------------------------------------------------===//
// Custom Lower
//===----------------------------------------------------------------------===//

SDValue M680x0TargetLowering::
LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  default: llvm_unreachable("Should not custom lower this!");
  case ISD::MUL:                return LowerMUL(Op, DAG);
  case ISD::SADDO:
  case ISD::UADDO:
  case ISD::SSUBO:
  case ISD::USUBO:
  case ISD::SMULO:
  case ISD::UMULO:              return LowerXALUO(Op, DAG);
  case ISD::SETCC:              return LowerSETCC(Op, DAG);
  case ISD::SETCCE:             return LowerSETCCE(Op, DAG);
  case ISD::SELECT:             return LowerSELECT(Op, DAG);
  case ISD::BRCOND:             return LowerBRCOND(Op, DAG);
  case ISD::ADDC:
  case ISD::ADDE:
  case ISD::SUBC:
  case ISD::SUBE:               return LowerADDC_ADDE_SUBC_SUBE(Op, DAG);
  case ISD::ConstantPool:       return LowerConstantPool(Op, DAG);
  case ISD::GlobalAddress:      return LowerGlobalAddress(Op, DAG);
  case ISD::ExternalSymbol:     return LowerExternalSymbol(Op, DAG);
  case ISD::BlockAddress:       return LowerBlockAddress(Op, DAG);
  case ISD::JumpTable:          return LowerJumpTable(Op, DAG);
  case ISD::VASTART:            return LowerVASTART(Op, DAG);
  case ISD::DYNAMIC_STACKALLOC: return LowerDYNAMIC_STACKALLOC(Op, DAG);
  }
}

SDValue M680x0TargetLowering::
LowerMUL(SDValue &N, SelectionDAG &DAG) const {
  EVT VT = N->getValueType(0);
  SDLoc DL(N);

  ConstantSDNode *C = dyn_cast<ConstantSDNode>(N->getOperand(1));

  if (C && isPowerOf2_64(C->getZExtValue())) {
    uint64_t MulAmt = C->getZExtValue();

    if (isPowerOf2_64(MulAmt))
      return DAG.getNode(ISD::SHL, DL, VT, N->getOperand(0),
                           DAG.getConstant(Log2_64(MulAmt), DL, MVT::i8));

    if (isPowerOf2_64(MulAmt - 1)) {
      // (mul x, 2^N + 1) => (add (shl x, N), x)
      return DAG.getNode(ISD::ADD, DL, VT, N->getOperand(0),
                         DAG.getNode(ISD::SHL, DL, VT, N->getOperand(0),
                         DAG.getConstant(Log2_64(MulAmt - 1), DL,
                         MVT::i8)));
    }

    if (isPowerOf2_64(MulAmt + 1)) {
      // (mul x, 2^N - 1) => (sub (shl x, N), x)
      return DAG.getNode(ISD::SUB, DL, VT, DAG.getNode(ISD::SHL, DL, VT,
                         N->getOperand(0),
                         DAG.getConstant(Log2_64(MulAmt + 1),
                         DL, MVT::i8)), N->getOperand(0));
    }
  }

  // These cannot be handle by M68000 and M68010
  if (!Subtarget.isM68020()) {
    SDValue LHS = N->getOperand(0);
    SDValue RHS = N->getOperand(1);
    if (VT == MVT::i32) {
      SDValue Args[] = { LHS, RHS };
      return makeLibCall( DAG, RTLIB::MUL_I32, VT, Args, true, DL).first;
    } else if (VT == MVT::i64) {
      unsigned LoSize = VT.getSizeInBits();
      SDValue HiLHS = DAG.getNode(ISD::SRA, DL, VT, LHS,
                      DAG.getConstant(LoSize - 1, DL,
                                      getPointerTy(DAG.getDataLayout())));
      SDValue HiRHS = DAG.getNode(ISD::SRA, DL, VT, RHS,
                      DAG.getConstant(LoSize - 1, DL,
                                      getPointerTy(DAG.getDataLayout())));
      SDValue Args[] = { HiLHS, LHS, HiRHS, RHS };
      SDValue Ret = makeLibCall( DAG, RTLIB::MUL_I64, VT, Args, true, DL).first;

      // We are intereseted in Lo part
      return DAG.getNode(ISD::EXTRACT_ELEMENT, DL, VT, Ret,
                         DAG.getIntPtrConstant(0, DL));
    }
  }

  // The rest is considered legal
  return SDValue();
}

SDValue M680x0TargetLowering::
LowerXALUO(SDValue Op, SelectionDAG &DAG) const {
  // Lower the "add/sub/mul with overflow" instruction into a regular ins plus
  // a "setcc" instruction that checks the overflow flag. The "brcond" lowering
  // looks for this combo and may remove the "setcc" instruction if the "setcc"
  // has only one use.
  SDNode *N = Op.getNode();
  SDValue LHS = N->getOperand(0);
  SDValue RHS = N->getOperand(1);
  unsigned BaseOp = 0;
  unsigned Cond = 0;
  SDLoc DL(Op);
  switch (Op.getOpcode()) {
  default: llvm_unreachable("Unknown ovf instruction!");
  case ISD::SADDO:
    BaseOp = M680x0ISD::ADD;
    Cond = M680x0::COND_VS;
    break;
  case ISD::UADDO:
    BaseOp = M680x0ISD::ADD;
    Cond = M680x0::COND_CS;
    break;
  case ISD::SSUBO:
    BaseOp = M680x0ISD::SUB;
    Cond = M680x0::COND_VS;
    break;
  case ISD::USUBO:
    BaseOp = M680x0ISD::SUB;
    Cond = M680x0::COND_CS;
    break;
  // case ISD::SMULO:
  //   BaseOp = M680x0ISD::SMUL;
  //   Cond = M680x0::COND_VS;
  //   break;
  // case ISD::UMULO: { // i64, i8 = umulo lhs, rhs --> i64, i64, i32 umul lhs,rhs
  //   SDVTList VTs =
  //     DAG.getVTList(N->getValueType(0), N->getValueType(0), MVT::i8);
  //   SDValue Sum = DAG.getNode(M680x0ISD::UMUL, DL, VTs, LHS, RHS);
  //
  //   SDValue SetCC =
  //     DAG.getNode(M680x0ISD::SETCC, DL, N->getValueType(1),
  //                 DAG.getConstant(M680x0::COND_VS, DL, MVT::i8),
  //                 SDValue(Sum.getNode(), 2));
  //
  //   return DAG.getNode(ISD::MERGE_VALUES, DL, N->getVTList(), Sum, SetCC);
  // }
  }

  // Also sets CCR.
  SDVTList VTs = DAG.getVTList(N->getValueType(0), MVT::i8);
  SDValue Sum = DAG.getNode(BaseOp, DL, VTs, LHS, RHS);

  SDValue SetCC =
    DAG.getNode(M680x0ISD::SETCC, DL, N->getValueType(1),
                DAG.getConstant(Cond, DL, MVT::i8),
                SDValue(Sum.getNode(), 1));

  return DAG.getNode(ISD::MERGE_VALUES, DL, N->getVTList(), Sum, SetCC);
}

/// Create a BT (Bit Test) node - Test bit \p BitNo in \p Src and set condition
/// according to equal/not-equal condition code \p CC.
static SDValue
getBitTestCondition(SDValue Src, SDValue BitNo, ISD::CondCode CC,
                    const SDLoc &DL, SelectionDAG &DAG) {
  // If Src is i8, promote it to i32 with any_extend.  There is no i8 BT
  // instruction.  Since the shift amount is in-range-or-undefined, we know
  // that doing a bittest on the i32 value is ok.
  if (Src.getValueType() == MVT::i8 || Src.getValueType() == MVT::i16)
    Src = DAG.getNode(ISD::ANY_EXTEND, DL, MVT::i32, Src);

  // If the operand types disagree, extend the shift amount to match.  Since
  // BT ignores high bits (like shifts) we can use anyextend.
  if (Src.getValueType() != BitNo.getValueType())
    BitNo = DAG.getNode(ISD::ANY_EXTEND, DL, Src.getValueType(), BitNo);

  SDValue BT = DAG.getNode(M680x0ISD::BT, DL, MVT::i32, Src, BitNo);

  // NOTE BTST sets CCR.Z flag
  M680x0::CondCode Cond = CC == ISD::SETEQ ? M680x0::COND_NE : M680x0::COND_EQ;
  return DAG.getNode(M680x0ISD::SETCC, DL, MVT::i8,
                     DAG.getConstant(Cond, DL, MVT::i8), BT);
}

/// Result of 'and' is compared against zero. Change to a BT node if possible.
static SDValue
LowerAndToBT(SDValue And, ISD::CondCode CC, const SDLoc &DL, SelectionDAG &DAG) {
  SDValue Op0 = And.getOperand(0);
  SDValue Op1 = And.getOperand(1);
  if (Op0.getOpcode() == ISD::TRUNCATE)
    Op0 = Op0.getOperand(0);
  if (Op1.getOpcode() == ISD::TRUNCATE)
    Op1 = Op1.getOperand(0);

  SDValue LHS, RHS;
  if (Op1.getOpcode() == ISD::SHL)
    std::swap(Op0, Op1);
  if (Op0.getOpcode() == ISD::SHL) {
    if (isOneConstant(Op0.getOperand(0))) {
      // If we looked past a truncate, check that it's only truncating away
      // known zeros.
      unsigned BitWidth = Op0.getValueSizeInBits();
      unsigned AndBitWidth = And.getValueSizeInBits();
      if (BitWidth > AndBitWidth) {
        APInt Zeros, Ones;
        DAG.computeKnownBits(Op0, Zeros, Ones);
        if (Zeros.countLeadingOnes() < BitWidth - AndBitWidth)
          return SDValue();
      }
      LHS = Op1;
      RHS = Op0.getOperand(1);
    }
  } else if (Op1.getOpcode() == ISD::Constant) {
    ConstantSDNode *AndRHS = cast<ConstantSDNode>(Op1);
    uint64_t AndRHSVal = AndRHS->getZExtValue();
    SDValue AndLHS = Op0;

    if (AndRHSVal == 1 && AndLHS.getOpcode() == ISD::SRL) {
      LHS = AndLHS.getOperand(0);
      RHS = AndLHS.getOperand(1);
    }

    // Use BT if the immediate can't be encoded in a TEST instruction.
    if (!isUInt<32>(AndRHSVal) && isPowerOf2_64(AndRHSVal)) {
      LHS = AndLHS;
      RHS = DAG.getConstant(Log2_64_Ceil(AndRHSVal), DL, LHS.getValueType());
    }
  }

  if (LHS.getNode())
    return getBitTestCondition(LHS, RHS, CC, DL, DAG);

  return SDValue();
}

static M680x0::CondCode
TranslateIntegerM680x0CC(ISD::CondCode SetCCOpcode) {
  switch (SetCCOpcode) {
  default: llvm_unreachable("Invalid integer condition!");
  case ISD::SETEQ:  return M680x0::COND_EQ;
  case ISD::SETGT:  return M680x0::COND_GT;
  case ISD::SETGE:  return M680x0::COND_GE;
  case ISD::SETLT:  return M680x0::COND_LT;
  case ISD::SETLE:  return M680x0::COND_LE;
  case ISD::SETNE:  return M680x0::COND_NE;
  case ISD::SETULT: return M680x0::COND_CS;
  case ISD::SETUGE: return M680x0::COND_CC;
  case ISD::SETUGT: return M680x0::COND_HI;
  case ISD::SETULE: return M680x0::COND_LS;
  }
}

/// Do a one-to-one translation of a ISD::CondCode to the M680x0-specific
/// condition code, returning the condition code and the LHS/RHS of the
/// comparison to make.
static unsigned
TranslateM680x0CC(ISD::CondCode SetCCOpcode, const SDLoc &DL,
               bool isFP, SDValue &LHS, SDValue &RHS, SelectionDAG &DAG) {
  if (!isFP) {
    if (ConstantSDNode *RHSC = dyn_cast<ConstantSDNode>(RHS)) {
      if (SetCCOpcode == ISD::SETGT && RHSC->isAllOnesValue()) {
        // X > -1   -> X == 0, jump !sign.
        RHS = DAG.getConstant(0, DL, RHS.getValueType());
        return M680x0::COND_PL;
      }
      if (SetCCOpcode == ISD::SETLT && RHSC->isNullValue()) {
        // X < 0   -> X == 0, jump on sign.
        return M680x0::COND_MI;
      }
      if (SetCCOpcode == ISD::SETLT && RHSC->getZExtValue() == 1) {
        // X < 1   -> X <= 0
        RHS = DAG.getConstant(0, DL, RHS.getValueType());
        return M680x0::COND_LE;
      }
    }

    return TranslateIntegerM680x0CC(SetCCOpcode);
  }

  // First determine if it is required or is profitable to flip the operands.

  // If LHS is a foldable load, but RHS is not, flip the condition.
  if (ISD::isNON_EXTLoad(LHS.getNode()) &&
      !ISD::isNON_EXTLoad(RHS.getNode())) {
    SetCCOpcode = getSetCCSwappedOperands(SetCCOpcode);
    std::swap(LHS, RHS);
  }

  switch (SetCCOpcode) {
  default: break;
  case ISD::SETOLT:
  case ISD::SETOLE:
  case ISD::SETUGT:
  case ISD::SETUGE:
    std::swap(LHS, RHS);
    break;
  }

  // On a floating point condition, the flags are set as follows:
  // ZF  PF  CF   op
  //  0 | 0 | 0 | X > Y
  //  0 | 0 | 1 | X < Y
  //  1 | 0 | 0 | X == Y
  //  1 | 1 | 1 | unordered
  switch (SetCCOpcode) {
  default: llvm_unreachable("Condcode should be pre-legalized away");
  case ISD::SETUEQ:
  case ISD::SETEQ:   return M680x0::COND_EQ;
  case ISD::SETOLT:              // flipped
  case ISD::SETOGT:
  case ISD::SETGT:   return M680x0::COND_HI;
  case ISD::SETOLE:              // flipped
  case ISD::SETOGE:
  case ISD::SETGE:   return M680x0::COND_CC;
  case ISD::SETUGT:              // flipped
  case ISD::SETULT:
  case ISD::SETLT:   return M680x0::COND_CS;
  case ISD::SETUGE:              // flipped
  case ISD::SETULE:
  case ISD::SETLE:   return M680x0::COND_LS;
  case ISD::SETONE:
  case ISD::SETNE:   return M680x0::COND_NE;
  // case ISD::SETUO:   return M680x0::COND_P;
  // case ISD::SETO:    return M680x0::COND_NP;
  case ISD::SETOEQ:
  case ISD::SETUNE:  return M680x0::COND_INVALID;
  }
}

// Convert (truncate (srl X, N) to i1) to (bt X, N)
static SDValue
LowerTruncateToBT(SDValue Op, ISD::CondCode CC,
                  const SDLoc &DL, SelectionDAG &DAG) {

  assert(Op.getOpcode() == ISD::TRUNCATE && Op.getValueType() == MVT::i1 &&
         "Expected TRUNCATE to i1 node");

  if (Op.getOperand(0).getOpcode() != ISD::SRL)
    return SDValue();

  SDValue ShiftRight = Op.getOperand(0);
  return getBitTestCondition(ShiftRight.getOperand(0), ShiftRight.getOperand(1),
                             CC, DL, DAG);
}

/// \brief return true if \c Op has a use that doesn't just read flags.
static bool
hasNonFlagsUse(SDValue Op) {
  for (SDNode::use_iterator UI = Op->use_begin(), UE = Op->use_end(); UI != UE;
       ++UI) {
    SDNode *User = *UI;
    unsigned UOpNo = UI.getOperandNo();
    if (User->getOpcode() == ISD::TRUNCATE && User->hasOneUse()) {
      // Look pass truncate.
      UOpNo = User->use_begin().getOperandNo();
      User = *User->use_begin();
    }

    if (User->getOpcode() != ISD::BRCOND && User->getOpcode() != ISD::SETCC &&
        !(User->getOpcode() == ISD::SELECT && UOpNo == 0))
      return true;
  }
  return false;
}

SDValue M680x0TargetLowering::
EmitTest(SDValue Op, unsigned M680x0CC, const SDLoc &DL,
         SelectionDAG &DAG) const {

  // CF and OF aren't always set the way we want. Determine which
  // of these we need.
  bool NeedCF = false;
  bool NeedOF = false;
  switch (M680x0CC) {
  default: break;
  case M680x0::COND_HI: case M680x0::COND_CC:
  case M680x0::COND_CS: case M680x0::COND_LS:
    NeedCF = true;
    break;
  case M680x0::COND_GT: case M680x0::COND_GE:
  case M680x0::COND_LT: case M680x0::COND_LE:
  case M680x0::COND_VS: case M680x0::COND_VC: {
    // Check if we really need to set the
    // Overflow flag. If NoSignedWrap is present
    // that is not actually needed.
    switch (Op->getOpcode()) {
    case ISD::ADD:
    case ISD::SUB:
    case ISD::MUL:
    case ISD::SHL: {
      const auto *BinNode = cast<BinaryWithFlagsSDNode>(Op.getNode());
      if (BinNode->Flags.hasNoSignedWrap())
        break;
    }
    default:
      NeedOF = true;
      break;
    }
    break;
  }
  }
  // See if we can use the CCR value from the operand instead of
  // doing a separate TEST. TEST always sets OF and CF to 0, so unless
  // we prove that the arithmetic won't overflow, we can't use OF or CF.
  if (Op.getResNo() != 0 || NeedOF || NeedCF) {
    // Emit a CMP with 0, which is the TEST pattern.
    return DAG.getNode(M680x0ISD::CMP, DL, MVT::i8,
        DAG.getConstant(0, DL, Op.getValueType()), Op);
  }
  unsigned Opcode = 0;
  unsigned NumOperands = 0;

  // Truncate operations may prevent the merge of the SETCC instruction
  // and the arithmetic instruction before it. Attempt to truncate the operands
  // of the arithmetic instruction and use a reduced bit-width instruction.
  bool NeedTruncation = false;
  SDValue ArithOp = Op;
  if (Op->getOpcode() == ISD::TRUNCATE && Op->hasOneUse()) {
    SDValue Arith = Op->getOperand(0);
    // Both the trunc and the arithmetic op need to have one user each.
    if (Arith->hasOneUse())
      switch (Arith.getOpcode()) {
        default: break;
        case ISD::ADD:
        case ISD::SUB:
        case ISD::AND:
        case ISD::OR:
        case ISD::XOR: {
          NeedTruncation = true;
          ArithOp = Arith;
        }
      }
  }

  // NOTICE: In the code below we use ArithOp to hold the arithmetic operation
  // which may be the result of a CAST.  We use the variable 'Op', which is the
  // non-casted variable when we check for possible users.
  switch (ArithOp.getOpcode()) {
  case ISD::ADD:
    Opcode = M680x0ISD::ADD;
    NumOperands = 2;
    break;
  case ISD::SHL:
  case ISD::SRL:
    // If we have a constant logical shift that's only used in a comparison
    // against zero turn it into an equivalent AND. This allows turning it into
    // a TEST instruction later.
    if ((M680x0CC == M680x0::COND_EQ || M680x0CC == M680x0::COND_NE) && Op->hasOneUse() &&
        isa<ConstantSDNode>(Op->getOperand(1)) && !hasNonFlagsUse(Op)) {
      EVT VT = Op.getValueType();
      unsigned BitWidth = VT.getSizeInBits();
      unsigned ShAmt = Op->getConstantOperandVal(1);
      if (ShAmt >= BitWidth) // Avoid undefined shifts.
        break;
      APInt Mask = ArithOp.getOpcode() == ISD::SRL
                       ? APInt::getHighBitsSet(BitWidth, BitWidth - ShAmt)
                       : APInt::getLowBitsSet(BitWidth, BitWidth - ShAmt);
      if (!Mask.isSignedIntN(32)) // Avoid large immediates.
        break;
      Op = DAG.getNode(ISD::AND, DL, VT, Op->getOperand(0),
                       DAG.getConstant(Mask, DL, VT));
    }
    break;

  case ISD::AND:
    // If the primary 'and' result isn't used, don't bother using M680x0ISD::AND,
    // because a TEST instruction will be better.
    if (!hasNonFlagsUse(Op)) {
      SDValue Op0 = ArithOp->getOperand(0);
      SDValue Op1 = ArithOp->getOperand(1);
      EVT VT = ArithOp.getValueType();
      bool isAndn = isBitwiseNot(Op0) || isBitwiseNot(Op1);
      bool isLegalAndnType = VT == MVT::i32 || VT == MVT::i64;

      // But if we can combine this into an ANDN operation, then create an AND
      // now and allow it to be pattern matched into an ANDN.
      if (/*!Subtarget.hasBMI() ||*/ !isAndn || !isLegalAndnType)
        break;
    }
    LLVM_FALLTHROUGH;
  case ISD::SUB:
  case ISD::OR:
  case ISD::XOR:
    // Due to the ISEL shortcoming noted above, be conservative if this op is
    // likely to be selected as part of a load-modify-store instruction.
    for (SDNode::use_iterator UI = Op.getNode()->use_begin(),
           UE = Op.getNode()->use_end(); UI != UE; ++UI)
      if (UI->getOpcode() == ISD::STORE)
        goto default_case;

    // Otherwise use a regular CCR-setting instruction.
    switch (ArithOp.getOpcode()) {
    default: llvm_unreachable("unexpected operator!");
    case ISD::SUB: Opcode = M680x0ISD::SUB; break;
    case ISD::XOR: Opcode = M680x0ISD::XOR; break;
    case ISD::AND: Opcode = M680x0ISD::AND; break;
    case ISD::OR:  Opcode = M680x0ISD::OR; break;
    }

    NumOperands = 2;
    break;
  case M680x0ISD::ADD:
  case M680x0ISD::SUB:
  case M680x0ISD::OR:
  case M680x0ISD::XOR:
  case M680x0ISD::AND:
    return SDValue(Op.getNode(), 1);
  default:
  default_case:
    break;
  }

  // If we found that truncation is beneficial, perform the truncation and
  // update 'Op'.
  if (NeedTruncation) {
    EVT VT = Op.getValueType();
    SDValue WideVal = Op->getOperand(0);
    EVT WideVT = WideVal.getValueType();
    unsigned ConvertedOp = 0;
    // Use a target machine opcode to prevent further DAGCombine
    // optimizations that may separate the arithmetic operations
    // from the setcc node.
    switch (WideVal.getOpcode()) {
      default: break;
      case ISD::ADD: ConvertedOp = M680x0ISD::ADD; break;
      case ISD::SUB: ConvertedOp = M680x0ISD::SUB; break;
      case ISD::AND: ConvertedOp = M680x0ISD::AND; break;
      case ISD::OR:  ConvertedOp = M680x0ISD::OR;  break;
      case ISD::XOR: ConvertedOp = M680x0ISD::XOR; break;
    }

    if (ConvertedOp) {
      const TargetLowering &TLI = DAG.getTargetLoweringInfo();
      if (TLI.isOperationLegal(WideVal.getOpcode(), WideVT)) {
        SDValue V0 = DAG.getNode(ISD::TRUNCATE, DL, VT, WideVal.getOperand(0));
        SDValue V1 = DAG.getNode(ISD::TRUNCATE, DL, VT, WideVal.getOperand(1));
        Op = DAG.getNode(ConvertedOp, DL, VT, V0, V1);
      }
    }
  }

  if (Opcode == 0) {
    // Emit a CMP with 0, which is the TEST pattern.
    return DAG.getNode(M680x0ISD::CMP, DL, MVT::i8,
        DAG.getConstant(0, DL, Op.getValueType()), Op);
  }
  SDVTList VTs = DAG.getVTList(Op.getValueType(), MVT::i8);
  SmallVector<SDValue, 4> Ops(Op->op_begin(), Op->op_begin() + NumOperands);

  SDValue New = DAG.getNode(Opcode, DL, VTs, Ops);
  DAG.ReplaceAllUsesWith(Op, New);
  return SDValue(New.getNode(), 1);
}

/// \brief Return true if the condition is an unsigned comparison operation.
static bool isM680x0CCUnsigned(unsigned M680x0CC) {
  switch (M680x0CC) {
  default:
    llvm_unreachable("Invalid integer condition!");
  case M680x0::COND_EQ:
  case M680x0::COND_NE:
  case M680x0::COND_CS:
  case M680x0::COND_HI:
  case M680x0::COND_LS:
  case M680x0::COND_CC:
    return true;
  case M680x0::COND_GT:
  case M680x0::COND_GE:
  case M680x0::COND_LT:
  case M680x0::COND_LE:
    return false;
  }
}

SDValue M680x0TargetLowering::
EmitCmp(SDValue Op0, SDValue Op1, unsigned M680x0CC,
        const SDLoc &DL, SelectionDAG &DAG) const {
  if (isNullConstant(Op1))
    return EmitTest(Op0, M680x0CC, DL, DAG);

  assert(!(isa<ConstantSDNode>(Op1) && Op0.getValueType() == MVT::i1) &&
         "Unexpected comparison operation for MVT::i1 operands");

  if ((Op0.getValueType() == MVT::i8 || Op0.getValueType() == MVT::i16 ||
       Op0.getValueType() == MVT::i32 || Op0.getValueType() == MVT::i64)) {
    // Only promote the compare up to I32 if it is a 16 bit operation
    // with an immediate.  16 bit immediates are to be avoided.
    if ((Op0.getValueType() == MVT::i16 &&
         (isa<ConstantSDNode>(Op0) || isa<ConstantSDNode>(Op1))) &&
        !DAG.getMachineFunction().getFunction()->optForMinSize()) {
      unsigned ExtendOp =
          isM680x0CCUnsigned(M680x0CC) ? ISD::ZERO_EXTEND : ISD::SIGN_EXTEND;
      Op0 = DAG.getNode(ExtendOp, DL, MVT::i32, Op0);
      Op1 = DAG.getNode(ExtendOp, DL, MVT::i32, Op1);
    }
    // Use SUB instead of CMP to enable CSE between SUB and CMP.
    SDVTList VTs = DAG.getVTList(Op0.getValueType(), MVT::i8);
    SDValue Sub = DAG.getNode(M680x0ISD::SUB, DL, VTs, Op0, Op1);
    return SDValue(Sub.getNode(), 1);
  }
  return DAG.getNode(M680x0ISD::CMP, DL, MVT::i8, Op0, Op1);
}

/// Result of 'and' or 'trunc to i1' is compared against zero.
/// Change to a BT node if possible.
SDValue M680x0TargetLowering::
LowerToBT(SDValue Op, ISD::CondCode CC,
          const SDLoc &DL, SelectionDAG &DAG) const {
  if (Op.getOpcode() == ISD::AND)
    return LowerAndToBT(Op, CC, DL, DAG);
  if (Op.getOpcode() == ISD::TRUNCATE && Op.getValueType() == MVT::i1)
    return LowerTruncateToBT(Op, CC, DL, DAG);
  return SDValue();
}

SDValue M680x0TargetLowering::
LowerSETCC(SDValue Op, SelectionDAG &DAG) const {
  MVT VT = Op.getSimpleValueType();
  assert(VT == MVT::i8 && "SetCC type must be 8-bit integer");

  SDValue Op0 = Op.getOperand(0);
  SDValue Op1 = Op.getOperand(1);
  SDLoc DL(Op);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();

  // Optimize to BT if possible.
  // Lower (X & (1 << N)) == 0 to BT(X, N).
  // Lower ((X >>u N) & 1) != 0 to BT(X, N).
  // Lower ((X >>s N) & 1) != 0 to BT(X, N).
  // Lower (trunc (X >> N) to i1) to BT(X, N).
  if (Op0.hasOneUse() && isNullConstant(Op1) &&
      (CC == ISD::SETEQ || CC == ISD::SETNE)) {
    if (SDValue NewSetCC = LowerToBT(Op0, CC, DL, DAG)) {
      if (VT == MVT::i1)
        return DAG.getNode(ISD::TRUNCATE, DL, MVT::i1, NewSetCC);
      return NewSetCC;
    }
  }

  // Look for X == 0, X == 1, X != 0, or X != 1.  We can simplify some forms of
  // these.
  if ((isOneConstant(Op1) || isNullConstant(Op1)) &&
      (CC == ISD::SETEQ || CC == ISD::SETNE)) {

    // If the input is a setcc, then reuse the input setcc or use a new one with
    // the inverted condition.
    if (Op0.getOpcode() == M680x0ISD::SETCC) {
      M680x0::CondCode CCode = (M680x0::CondCode)Op0.getConstantOperandVal(0);
      bool Invert = (CC == ISD::SETNE) ^ isNullConstant(Op1);
      if (!Invert)
        return Op0;

      CCode = M680x0::GetOppositeBranchCondition(CCode);
      SDValue SetCC = DAG.getNode(M680x0ISD::SETCC, DL, MVT::i8,
                                  DAG.getConstant(CCode, DL, MVT::i8),
                                  Op0.getOperand(1));
      if (VT == MVT::i1)
        return DAG.getNode(ISD::TRUNCATE, DL, MVT::i1, SetCC);
      return SetCC;
    }
  }
  if (Op0.getValueType() == MVT::i1 && (CC == ISD::SETEQ || CC == ISD::SETNE)) {
    if (isOneConstant(Op1)) {
      ISD::CondCode NewCC = ISD::getSetCCInverse(CC, true);
      return DAG.getSetCC(DL, VT, Op0, DAG.getConstant(0, DL, MVT::i1), NewCC);
    }
    if (!isNullConstant(Op1)) {
      SDValue Xor = DAG.getNode(ISD::XOR, DL, MVT::i1, Op0, Op1);
      return DAG.getSetCC(DL, VT, Xor, DAG.getConstant(0, DL, MVT::i1), CC);
    }
  }

  bool isFP = Op1.getSimpleValueType().isFloatingPoint();
  unsigned M680x0CC = TranslateM680x0CC(CC, DL, isFP, Op0, Op1, DAG);
  if (M680x0CC == M680x0::COND_INVALID)
    return SDValue();

  SDValue CCR = EmitCmp(Op0, Op1, M680x0CC, DL, DAG);
  // CCR = ConvertCmpIfNecessary(CCR, DAG);
  return DAG.getNode(M680x0ISD::SETCC, DL, MVT::i8,
                     DAG.getConstant(M680x0CC, DL, MVT::i8), CCR);
}

SDValue M680x0TargetLowering::
LowerSETCCE(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue Carry = Op.getOperand(2);
  SDValue Cond = Op.getOperand(3);
  SDLoc DL(Op);

  assert(LHS.getSimpleValueType().isInteger() && "SETCCE is integer only.");
  M680x0::CondCode CC = TranslateIntegerM680x0CC(cast<CondCodeSDNode>(Cond)->get());

  assert(Carry.getOpcode() != ISD::CARRY_FALSE);
  SDVTList VTs = DAG.getVTList(LHS.getValueType(), MVT::i32);
  SDValue Cmp = DAG.getNode(M680x0ISD::SUBX, DL, VTs, LHS, RHS, Carry);
  SDValue SetCC = DAG.getNode(M680x0ISD::SETCC, DL, MVT::i8,
                              DAG.getConstant(CC, DL, MVT::i8), Cmp.getValue(1));
  if (Op.getSimpleValueType() == MVT::i1)
      return DAG.getNode(ISD::TRUNCATE, DL, MVT::i1, SetCC);
  return SetCC;
}

/// Return true if opcode is a M680x0 logical comparison.
static bool
isM680x0LogicalCmp(SDValue Op) {
  unsigned Opc = Op.getNode()->getOpcode();
  if (Opc == M680x0ISD::CMP)
    return true;
  if (Op.getResNo() == 1 &&
      (Opc == M680x0ISD::ADD ||
       Opc == M680x0ISD::SUB ||
       Opc == M680x0ISD::ADDX ||
       Opc == M680x0ISD::SUBX ||
       Opc == M680x0ISD::SMUL ||
       Opc == M680x0ISD::UMUL ||
       Opc == M680x0ISD::OR ||
       Opc == M680x0ISD::XOR ||
       Opc == M680x0ISD::AND))
    return true;

  if (Op.getResNo() == 2 && Opc == M680x0ISD::UMUL)
    return true;

  return false;
}

static bool
isTruncWithZeroHighBitsInput(SDValue V, SelectionDAG &DAG) {
  if (V.getOpcode() != ISD::TRUNCATE)
    return false;

  SDValue VOp0 = V.getOperand(0);
  unsigned InBits = VOp0.getValueSizeInBits();
  unsigned Bits = V.getValueSizeInBits();
  return DAG.MaskedValueIsZero(VOp0, APInt::getHighBitsSet(InBits,InBits-Bits));
}

SDValue M680x0TargetLowering::
LowerSELECT(SDValue Op, SelectionDAG &DAG) const {
  bool addTest = true;
  SDValue Cond  = Op.getOperand(0);
  SDValue Op1 = Op.getOperand(1);
  SDValue Op2 = Op.getOperand(2);
  SDLoc DL(Op);
  SDValue CC;

  if (Cond.getOpcode() == ISD::SETCC) {
    if (SDValue NewCond = LowerSETCC(Cond, DAG))
      Cond = NewCond;
  }

  // (select (x == 0), -1, y) -> (sign_bit (x - 1)) | y
  // (select (x == 0), y, -1) -> ~(sign_bit (x - 1)) | y
  // (select (x != 0), y, -1) -> (sign_bit (x - 1)) | y
  // (select (x != 0), -1, y) -> ~(sign_bit (x - 1)) | y
  if (Cond.getOpcode() == M680x0ISD::SETCC &&
      Cond.getOperand(1).getOpcode() == M680x0ISD::CMP &&
      isNullConstant(Cond.getOperand(1).getOperand(0))) {
    SDValue Cmp = Cond.getOperand(1);

    unsigned CondCode =cast<ConstantSDNode>(Cond.getOperand(0))->getZExtValue();

    if ((isAllOnesConstant(Op1) || isAllOnesConstant(Op2)) &&
        (CondCode == M680x0::COND_EQ || CondCode == M680x0::COND_NE)) {
      SDValue Y = isAllOnesConstant(Op2) ? Op1 : Op2;

      SDValue CmpOp0 = Cmp.getOperand(1);
      // Apply further optimizations for special cases
      // (select (x != 0), -1, 0) -> neg & sbb
      // (select (x == 0), 0, -1) -> neg & sbb
      if (isNullConstant(Y) &&
         (isAllOnesConstant(Op1) == (CondCode == M680x0::COND_NE))) {

          SDVTList VTs = DAG.getVTList(CmpOp0.getValueType(), MVT::i32);

          SDValue Neg = DAG.getNode(M680x0ISD::SUB, DL, VTs,
                        DAG.getConstant(0, DL, CmpOp0.getValueType()), CmpOp0);

          SDValue Res = DAG.getNode(M680x0ISD::SETCC_CARRY, DL, Op.getValueType(),
                                    DAG.getConstant(M680x0::COND_CS, DL, MVT::i8),
                                    SDValue(Neg.getNode(), 1));
          return Res;
        }

      Cmp = DAG.getNode(M680x0ISD::CMP, DL, MVT::i8,
          DAG.getConstant(1, DL, CmpOp0.getValueType()), CmpOp0);
      // Cmp = ConvertCmpIfNecessary(Cmp, DAG);

      SDValue Res =   // Res = 0 or -1.
        DAG.getNode(M680x0ISD::SETCC_CARRY, DL, Op.getValueType(),
                    DAG.getConstant(M680x0::COND_CS, DL, MVT::i8), Cmp);

      if (isAllOnesConstant(Op1) != (CondCode == M680x0::COND_EQ))
        Res = DAG.getNOT(DL, Res, Res.getValueType());

      if (!isNullConstant(Op2))
        Res = DAG.getNode(ISD::OR, DL, Res.getValueType(), Res, Y);
      return Res;
    }
  }

  // Look past (and (setcc_carry (cmp ...)), 1).
  if (Cond.getOpcode() == ISD::AND &&
      Cond.getOperand(0).getOpcode() == M680x0ISD::SETCC_CARRY &&
      isOneConstant(Cond.getOperand(1)))
    Cond = Cond.getOperand(0);

  // If condition flag is set by a M680x0ISD::CMP, then use it as the condition
  // setting operand in place of the M680x0ISD::SETCC.
  unsigned CondOpcode = Cond.getOpcode();
  if (CondOpcode == M680x0ISD::SETCC ||
      CondOpcode == M680x0ISD::SETCC_CARRY) {
    CC = Cond.getOperand(0);

    SDValue Cmp = Cond.getOperand(1);
    unsigned Opc = Cmp.getOpcode();

    bool IllegalFPCMov = false;

    if ((isM680x0LogicalCmp(Cmp) && !IllegalFPCMov) || Opc == M680x0ISD::BT) { // FIXME
      Cond = Cmp;
      addTest = false;
    }
  } else if (CondOpcode == ISD::USUBO || CondOpcode == ISD::SSUBO ||
             CondOpcode == ISD::UADDO || CondOpcode == ISD::SADDO ||
             CondOpcode == ISD::UMULO || CondOpcode == ISD::SMULO) {
    SDValue LHS = Cond.getOperand(0);
    SDValue RHS = Cond.getOperand(1);
    unsigned MxOpcode;
    unsigned MxCond;
    SDVTList VTs;
    switch (CondOpcode) {
    case ISD::UADDO: MxOpcode = M680x0ISD::ADD;  MxCond = M680x0::COND_CS; break;
    case ISD::SADDO: MxOpcode = M680x0ISD::ADD;  MxCond = M680x0::COND_VS; break;
    case ISD::USUBO: MxOpcode = M680x0ISD::SUB;  MxCond = M680x0::COND_CS; break;
    case ISD::SSUBO: MxOpcode = M680x0ISD::SUB;  MxCond = M680x0::COND_VS; break;
    case ISD::UMULO: MxOpcode = M680x0ISD::UMUL; MxCond = M680x0::COND_VS; break;
    case ISD::SMULO: MxOpcode = M680x0ISD::SMUL; MxCond = M680x0::COND_VS; break;
    default: llvm_unreachable("unexpected overflowing operator");
    }
    if (CondOpcode == ISD::UMULO)
      VTs = DAG.getVTList(LHS.getValueType(), LHS.getValueType(),
                          MVT::i32);
    else
      VTs = DAG.getVTList(LHS.getValueType(), MVT::i32);

    SDValue MxOp = DAG.getNode(MxOpcode, DL, VTs, LHS, RHS);

    if (CondOpcode == ISD::UMULO)
      Cond = MxOp.getValue(2);
    else
      Cond = MxOp.getValue(1);

    CC = DAG.getConstant(MxCond, DL, MVT::i8);
    addTest = false;
  }

  if (addTest) {
    // Look past the truncate if the high bits are known zero.
    if (isTruncWithZeroHighBitsInput(Cond, DAG))
      Cond = Cond.getOperand(0);

    // We know the result of AND is compared against zero. Try to match
    // it to BT.
    if (Cond.getOpcode() == ISD::AND && Cond.hasOneUse()) {
      if (SDValue NewSetCC = LowerToBT(Cond, ISD::SETNE, DL, DAG)) {
        CC = NewSetCC.getOperand(0);
        Cond = NewSetCC.getOperand(1);
        addTest = false;
      }
    }
  }

  if (addTest) {
    CC = DAG.getConstant(M680x0::COND_NE, DL, MVT::i8);
    Cond = EmitTest(Cond, M680x0::COND_NE, DL, DAG);
  }

  // a <  b ? -1 :  0 -> RES = ~setcc_carry
  // a <  b ?  0 : -1 -> RES = setcc_carry
  // a >= b ? -1 :  0 -> RES = setcc_carry
  // a >= b ?  0 : -1 -> RES = ~setcc_carry
  if (Cond.getOpcode() == M680x0ISD::SUB) {
    // Cond = ConvertCmpIfNecessary(Cond, DAG);
    unsigned CondCode = cast<ConstantSDNode>(CC)->getZExtValue();

    if ((CondCode == M680x0::COND_CC || CondCode == M680x0::COND_CS) &&
        (isAllOnesConstant(Op1) || isAllOnesConstant(Op2)) &&
        (isNullConstant(Op1) || isNullConstant(Op2))) {
      SDValue Res = DAG.getNode(M680x0ISD::SETCC_CARRY, DL, Op.getValueType(),
                                DAG.getConstant(M680x0::COND_CS, DL, MVT::i8),
                                Cond);
      if (isAllOnesConstant(Op1) != (CondCode == M680x0::COND_CS))
        return DAG.getNOT(DL, Res, Res.getValueType());
      return Res;
    }
  }

  // M680x0 doesn't have an i8 cmov. If both operands are the result of a truncate
  // widen the cmov and push the truncate through. This avoids introducing a new
  // branch during isel and doesn't add any extensions.
  if (Op.getValueType() == MVT::i8 &&
      Op1.getOpcode() == ISD::TRUNCATE && Op2.getOpcode() == ISD::TRUNCATE) {
    SDValue T1 = Op1.getOperand(0), T2 = Op2.getOperand(0);
    if (T1.getValueType() == T2.getValueType() &&
        // Blacklist CopyFromReg to avoid partial register stalls.
        T1.getOpcode() != ISD::CopyFromReg && T2.getOpcode()!=ISD::CopyFromReg){
      SDVTList VTs = DAG.getVTList(T1.getValueType(), MVT::Glue);
      SDValue Cmov = DAG.getNode(M680x0ISD::CMOV, DL, VTs, T2, T1, CC, Cond);
      return DAG.getNode(ISD::TRUNCATE, DL, Op.getValueType(), Cmov);
    }
  }

  // M680x0ISD::CMOV means set the result (which is operand 1) to the RHS if
  // condition is true.
  SDVTList VTs = DAG.getVTList(Op.getValueType(), MVT::Glue);
  SDValue Ops[] = { Op2, Op1, CC, Cond };
  return DAG.getNode(M680x0ISD::CMOV, DL, VTs, Ops);
}

/// Return true if node is an ISD::AND or ISD::OR of two M680x0::SETcc nodes
/// each of which has no other use apart from the AND / OR.
static bool isAndOrOfSetCCs(SDValue Op, unsigned &Opc) {
  Opc = Op.getOpcode();
  if (Opc != ISD::OR && Opc != ISD::AND)
    return false;
  return (M680x0::IsSETCC(Op.getOperand(0).getOpcode()) &&
          Op.getOperand(0).hasOneUse() &&
          M680x0::IsSETCC(Op.getOperand(1).getOpcode()) &&
          Op.getOperand(1).hasOneUse());
}

/// Return true if node is an ISD::XOR of a M680x0ISD::SETCC and 1 and that the
/// SETCC node has a single use.
static bool isXor1OfSetCC(SDValue Op) {
  if (Op.getOpcode() != ISD::XOR)
    return false;
  if (isOneConstant(Op.getOperand(1)))
    return Op.getOperand(0).getOpcode() == M680x0ISD::SETCC &&
           Op.getOperand(0).hasOneUse();
  return false;
}

SDValue M680x0TargetLowering::
LowerBRCOND(SDValue Op, SelectionDAG &DAG) const {
  bool addTest = true;
  SDValue Chain = Op.getOperand(0);
  SDValue Cond  = Op.getOperand(1);
  SDValue Dest  = Op.getOperand(2);
  SDLoc DL(Op);
  SDValue CC;
  bool Inverted = false;

  if (Cond.getOpcode() == ISD::SETCC) {
    // Check for setcc([su]{add,sub,mul}o == 0).
    if (cast<CondCodeSDNode>(Cond.getOperand(2))->get() == ISD::SETEQ &&
        isNullConstant(Cond.getOperand(1)) &&
        Cond.getOperand(0).getResNo() == 1 &&
        (Cond.getOperand(0).getOpcode() == ISD::SADDO ||
         Cond.getOperand(0).getOpcode() == ISD::UADDO ||
         Cond.getOperand(0).getOpcode() == ISD::SSUBO ||
         Cond.getOperand(0).getOpcode() == ISD::USUBO /*||
         Cond.getOperand(0).getOpcode() == ISD::SMULO ||
         Cond.getOperand(0).getOpcode() == ISD::UMULO)*/)) {
      Inverted = true;
      Cond = Cond.getOperand(0);
    } else {
      if (SDValue NewCond = LowerSETCC(Cond, DAG))
        Cond = NewCond;
    }
  }
#if 0
  // FIXME: LowerXALUO doesn't handle these!!
  else if (Cond.getOpcode() == M680x0ISD::ADD  ||
           Cond.getOpcode() == M680x0ISD::SUB  ||
           Cond.getOpcode() == M680x0ISD::SMUL ||
           Cond.getOpcode() == M680x0ISD::UMUL)
    Cond = LowerXALUO(Cond, DAG);
#endif

  // Look pass (and (setcc_carry (cmp ...)), 1).
  if (Cond.getOpcode() == ISD::AND &&
      Cond.getOperand(0).getOpcode() == M680x0ISD::SETCC_CARRY &&
      isOneConstant(Cond.getOperand(1)))
    Cond = Cond.getOperand(0);

  // If condition flag is set by a M680x0ISD::CMP, then use it as the condition
  // setting operand in place of the M680x0ISD::SETCC.
  unsigned CondOpcode = Cond.getOpcode();
  if (CondOpcode == M680x0ISD::SETCC ||
      CondOpcode == M680x0ISD::SETCC_CARRY) {
    CC = Cond.getOperand(0);

    SDValue Cmp = Cond.getOperand(1);
    unsigned Opc = Cmp.getOpcode();
    // FIXME: WHY THE SPECIAL CASING OF LogicalCmp??
    if (isM680x0LogicalCmp(Cmp) || Opc == M680x0ISD::BT) {
      Cond = Cmp;
      addTest = false;
    } else {
      switch (cast<ConstantSDNode>(CC)->getZExtValue()) {
      default: break;
      case M680x0::COND_VS:
      case M680x0::COND_CS:
        // These can only come from an arithmetic instruction with overflow,
        // e.g. SADDO, UADDO.
        Cond = Cond.getNode()->getOperand(1);
        addTest = false;
        break;
      }
    }
  }
  CondOpcode = Cond.getOpcode();
  if (CondOpcode == ISD::UADDO || CondOpcode == ISD::SADDO ||
      CondOpcode == ISD::USUBO || CondOpcode == ISD::SSUBO /*||
      CondOpcode == ISD::UMULO || CondOpcode == ISD::SMULO*/) {
    SDValue LHS = Cond.getOperand(0);
    SDValue RHS = Cond.getOperand(1);
    unsigned MxOpcode;
    unsigned MxCond;
    SDVTList VTs;
    // Keep this in sync with LowerXALUO, otherwise we might create redundant
    // instructions that can't be removed afterwards (i.e. M680x0ISD::ADD and
    // M680x0ISD::INC).
    switch (CondOpcode) {
    case ISD::UADDO: MxOpcode = M680x0ISD::ADD;  MxCond = M680x0::COND_CS; break;
    case ISD::SADDO: MxOpcode = M680x0ISD::ADD;  MxCond = M680x0::COND_VS; break;
    case ISD::USUBO: MxOpcode = M680x0ISD::SUB;  MxCond = M680x0::COND_CS; break;
    case ISD::SSUBO: MxOpcode = M680x0ISD::SUB;  MxCond = M680x0::COND_VS; break;
    case ISD::UMULO: MxOpcode = M680x0ISD::UMUL; MxCond = M680x0::COND_VS; break;
    case ISD::SMULO: MxOpcode = M680x0ISD::SMUL; MxCond = M680x0::COND_VS; break;
    default: llvm_unreachable("unexpected overflowing operator");
    }

    if (Inverted)
      MxCond = M680x0::GetOppositeBranchCondition((M680x0::CondCode)MxCond);

    if (CondOpcode == ISD::UMULO)
      VTs = DAG.getVTList(LHS.getValueType(), LHS.getValueType(), MVT::i8);
    else
      VTs = DAG.getVTList(LHS.getValueType(), MVT::i8);

    SDValue MxOp = DAG.getNode(MxOpcode, DL, VTs, LHS, RHS);

    if (CondOpcode == ISD::UMULO)
      Cond = MxOp.getValue(2);
    else
      Cond = MxOp.getValue(1);

    CC = DAG.getConstant(MxCond, DL, MVT::i8);
    addTest = false;
  } else {
    unsigned CondOpc;
    if (Cond.hasOneUse() && isAndOrOfSetCCs(Cond, CondOpc)) {
      SDValue Cmp = Cond.getOperand(0).getOperand(1);
      if (CondOpc == ISD::OR) {
        // Also, recognize the pattern generated by an FCMP_UNE. We can emit
        // two branches instead of an explicit OR instruction with a
        // separate test.
        if (Cmp == Cond.getOperand(1).getOperand(1) &&
            isM680x0LogicalCmp(Cmp)) {
          CC = Cond.getOperand(0).getOperand(0);
          Chain = DAG.getNode(M680x0ISD::BRCOND, DL, Op.getValueType(),
                              Chain, Dest, CC, Cmp);
          CC = Cond.getOperand(1).getOperand(0);
          Cond = Cmp;
          addTest = false;
        }
      } else { // ISD::AND
        // Also, recognize the pattern generated by an FCMP_OEQ. We can emit
        // two branches instead of an explicit AND instruction with a
        // separate test. However, we only do this if this block doesn't
        // have a fall-through edge, because this requires an explicit
        // jmp when the condition is false.
        if (Cmp == Cond.getOperand(1).getOperand(1) &&
            isM680x0LogicalCmp(Cmp) &&
            Op.getNode()->hasOneUse()) {
          M680x0::CondCode CCode =
            (M680x0::CondCode)Cond.getOperand(0).getConstantOperandVal(0);
          CCode = M680x0::GetOppositeBranchCondition(CCode);
          CC = DAG.getConstant(CCode, DL, MVT::i8);
          SDNode *User = *Op.getNode()->use_begin();
          // Look for an unconditional branch following this conditional branch.
          // We need this because we need to reverse the successors in order
          // to implement FCMP_OEQ.
          if (User->getOpcode() == ISD::BR) {
            SDValue FalseBB = User->getOperand(1);
            SDNode *NewBR =
              DAG.UpdateNodeOperands(User, User->getOperand(0), Dest);
            assert(NewBR == User);
            (void)NewBR;
            Dest = FalseBB;

            Chain = DAG.getNode(M680x0ISD::BRCOND, DL, Op.getValueType(),
                                Chain, Dest, CC, Cmp);
            M680x0::CondCode CCode =
              (M680x0::CondCode)Cond.getOperand(1).getConstantOperandVal(0);
            CCode = M680x0::GetOppositeBranchCondition(CCode);
            CC = DAG.getConstant(CCode, DL, MVT::i8);
            Cond = Cmp;
            addTest = false;
          }
        }
      }
    } else if (Cond.hasOneUse() && isXor1OfSetCC(Cond)) {
      // Recognize for xorb (setcc), 1 patterns. The xor inverts the condition.
      // It should be transformed during dag combiner except when the condition
      // is set by a arithmetics with overflow node.
      M680x0::CondCode CCode =
        (M680x0::CondCode)Cond.getOperand(0).getConstantOperandVal(0);
      CCode = M680x0::GetOppositeBranchCondition(CCode);
      CC = DAG.getConstant(CCode, DL, MVT::i8);
      Cond = Cond.getOperand(0).getOperand(1);
      addTest = false;
    } /*else if (Cond.getOpcode() == ISD::SETCC &&
               cast<CondCodeSDNode>(Cond.getOperand(2))->get() == ISD::SETOEQ) {
      // For FCMP_OEQ, we can emit
      // two branches instead of an explicit AND instruction with a
      // separate test. However, we only do this if this block doesn't
      // have a fall-through edge, because this requires an explicit
      // jmp when the condition is false.
      if (Op.getNode()->hasOneUse()) {
        SDNode *User = *Op.getNode()->use_begin();
        // Look for an unconditional branch following this conditional branch.
        // We need this because we need to reverse the successors in order
        // to implement FCMP_OEQ.
        if (User->getOpcode() == ISD::BR) {
          SDValue FalseBB = User->getOperand(1);
          SDNode *NewBR =
            DAG.UpdateNodeOperands(User, User->getOperand(0), Dest);
          assert(NewBR == User);
          (void)NewBR;
          Dest = FalseBB;

          SDValue Cmp = DAG.getNode(M680x0ISD::CMP, DL, MVT::i32,
                                    Cond.getOperand(0), Cond.getOperand(1));
          // Cmp = ConvertCmpIfNecessary(Cmp, DAG);
          CC = DAG.getConstant(M680x0::COND_NE, DL, MVT::i8);
          Chain = DAG.getNode(M680x0ISD::BRCOND, DL, Op.getValueType(),
                              Chain, Dest, CC, Cmp);
          CC = DAG.getConstant(M680x0::COND_P, DL, MVT::i8);
          Cond = Cmp;
          addTest = false;
        }
      }
    } else if (Cond.getOpcode() == ISD::SETCC &&
               cast<CondCodeSDNode>(Cond.getOperand(2))->get() == ISD::SETUNE) {
      // For FCMP_UNE, we can emit
      // two branches instead of an explicit AND instruction with a
      // separate test. However, we only do this if this block doesn't
      // have a fall-through edge, because this requires an explicit
      // jmp when the condition is false.
      if (Op.getNode()->hasOneUse()) {
        SDNode *User = *Op.getNode()->use_begin();
        // Look for an unconditional branch following this conditional branch.
        // We need this because we need to reverse the successors in order
        // to implement FCMP_UNE.
        if (User->getOpcode() == ISD::BR) {
          SDValue FalseBB = User->getOperand(1);
          SDNode *NewBR =
            DAG.UpdateNodeOperands(User, User->getOperand(0), Dest);
          assert(NewBR == User);
          (void)NewBR;

          SDValue Cmp = DAG.getNode(M680x0ISD::CMP, DL, MVT::i32,
                                    Cond.getOperand(0), Cond.getOperand(1));
          Cmp = ConvertCmpIfNecessary(Cmp, DAG);
          CC = DAG.getConstant(M680x0::COND_NE, DL, MVT::i8);
          Chain = DAG.getNode(M680x0ISD::BRCOND, DL, Op.getValueType(),
                              Chain, Dest, CC, Cmp);
          CC = DAG.getConstant(M680x0::COND_NP, DL, MVT::i8);
          Cond = Cmp;
          addTest = false;
          Dest = FalseBB;
        }
      }
    } */
  }

  if (addTest) {
    // Look pass the truncate if the high bits are known zero.
    if (isTruncWithZeroHighBitsInput(Cond, DAG))
        Cond = Cond.getOperand(0);

    // We know the result is compared against zero. Try to match it to BT.
    if (Cond.hasOneUse()) {
      if (SDValue NewSetCC = LowerToBT(Cond, ISD::SETNE, DL, DAG)) {
        CC = NewSetCC.getOperand(0);
        Cond = NewSetCC.getOperand(1);
        addTest = false;
      }
    }
  }

  if (addTest) {
    M680x0::CondCode MxCond = Inverted ? M680x0::COND_EQ : M680x0::COND_NE;
    CC = DAG.getConstant(MxCond, DL, MVT::i8);
    Cond = EmitTest(Cond, MxCond, DL, DAG);
  }
  // Cond = ConvertCmpIfNecessary(Cond, DAG);
  return DAG.getNode(M680x0ISD::BRCOND, DL, Op.getValueType(),
                     Chain, Dest, CC, Cond);
}

SDValue M680x0TargetLowering::
LowerADDC_ADDE_SUBC_SUBE(SDValue Op, SelectionDAG &DAG) const {
  MVT VT = Op.getNode()->getSimpleValueType(0);

  // Let legalize expand this if it isn't a legal type yet.
  if (!DAG.getTargetLoweringInfo().isTypeLegal(VT))
    return SDValue();

  SDVTList VTs = DAG.getVTList(VT, MVT::i8);

  unsigned Opc;
  bool ExtraOp = false;
  switch (Op.getOpcode()) {
  default: llvm_unreachable("Invalid code");
  case ISD::ADDC: Opc = M680x0ISD::ADD;  break;
  case ISD::ADDE: Opc = M680x0ISD::ADDX; ExtraOp = true; break;
  case ISD::SUBC: Opc = M680x0ISD::SUB;  break;
  case ISD::SUBE: Opc = M680x0ISD::SUBX; ExtraOp = true; break;
  }

  if (!ExtraOp)
    return DAG.getNode(Opc, SDLoc(Op), VTs, Op.getOperand(0),
                       Op.getOperand(1));
  return DAG.getNode(Opc, SDLoc(Op), VTs, Op.getOperand(0),
                     Op.getOperand(1), Op.getOperand(2));
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

  // In PIC mode (unless we're in PCRel PIC mode) we add an offset to the
  // global base reg.
  unsigned char OpFlag = Subtarget.classifyLocalReference(nullptr);

  unsigned WrapperKind = M680x0ISD::Wrapper;
  if (M680x0II::isPCRelGlobalReference(OpFlag)) {
    WrapperKind = M680x0ISD::WrapperPC;
  }

  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result = DAG.getTargetConstantPool(
      CP->getConstVal(), PtrVT, CP->getAlignment(), CP->getOffset(), OpFlag);

  SDLoc DL(CP);
  Result = DAG.getNode(WrapperKind, DL, PtrVT, Result);

  // With PIC, the address is actually $g + Offset.
  if (M680x0II::isGlobalRelativeToPICBase(OpFlag)) {
    Result = DAG.getNode(ISD::ADD, DL, PtrVT,
                 DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(), PtrVT), Result);
  }

  return Result;
}

SDValue M680x0TargetLowering::
LowerExternalSymbol(SDValue Op, SelectionDAG &DAG) const {
  const char *Sym = cast<ExternalSymbolSDNode>(Op)->getSymbol();

  // In PIC mode (unless we're in PCRel PIC mode) we add an offset to the
  // global base reg.
  const Module *Mod = DAG.getMachineFunction().getFunction()->getParent();
  unsigned char OpFlag = Subtarget.classifyExternalReference(*Mod);

  unsigned WrapperKind = M680x0ISD::Wrapper;
  if (M680x0II::isPCRelGlobalReference(OpFlag)) {
    WrapperKind = M680x0ISD::WrapperPC;
  }

  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result = DAG.getTargetExternalSymbol(Sym, PtrVT, OpFlag);

  SDLoc DL(Op);
  Result = DAG.getNode(WrapperKind, DL, PtrVT, Result);

  // With PIC, the address is actually $g + Offset.
  if (M680x0II::isGlobalRelativeToPICBase(OpFlag)) {
    Result = DAG.getNode(ISD::ADD, DL, PtrVT,
                 DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(), PtrVT), Result);
  }

  // For symbols that require a load from a stub to get the address, emit the
  // load.
  if (M680x0II::isGlobalStubReference(OpFlag)) {
    Result = DAG.getLoad(PtrVT, DL, DAG.getEntryNode(), Result,
                 MachinePointerInfo::getGOT(DAG.getMachineFunction()));
  }

  return Result;
}

SDValue M680x0TargetLowering::
LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const {
  unsigned char OpFlags = Subtarget.classifyBlockAddressReference();
  const BlockAddress *BA = cast<BlockAddressSDNode>(Op)->getBlockAddress();
  int64_t Offset = cast<BlockAddressSDNode>(Op)->getOffset();
  SDLoc DL(Op);
  auto PtrVT = getPointerTy(DAG.getDataLayout());

  // Create the TargetBlockAddressAddress node.
  SDValue Result = DAG.getTargetBlockAddress(BA, PtrVT, Offset, OpFlags);

  if (M680x0II::isPCRelBlockReference(OpFlags)) {
    Result = DAG.getNode(M680x0ISD::WrapperPC, DL, PtrVT, Result);
  } else {
    Result = DAG.getNode(M680x0ISD::Wrapper, DL, PtrVT, Result);
  }

  // With PIC, the address is actually $g + Offset.
  if (M680x0II::isGlobalRelativeToPICBase(OpFlags)) {
    Result = DAG.getNode(ISD::ADD, DL, PtrVT,
                 DAG.getNode(M680x0ISD::GlobalBaseReg, DL, PtrVT), Result);
  }

  return Result;
}

SDValue M680x0TargetLowering::
LowerGlobalAddress(const GlobalValue *GV, const SDLoc &DL, int64_t Offset,
                   SelectionDAG &DAG) const {
  unsigned char OpFlags = Subtarget.classifyGlobalReference(GV);
  auto PtrVT = getPointerTy(DAG.getDataLayout());

  // Create the TargetGlobalAddress node, folding in the constant
  // offset if it is legal.
  SDValue Result;
  if (M680x0II::isDirectGlobalReference(OpFlags)) {
    Result = DAG.getTargetGlobalAddress(GV, DL, PtrVT, Offset);
    Offset = 0;
  } else {
    Result = DAG.getTargetGlobalAddress(GV, DL, PtrVT, 0, OpFlags);
  }

  if (M680x0II::isPCRelGlobalReference(OpFlags))
    Result = DAG.getNode(M680x0ISD::WrapperPC, DL, PtrVT, Result);
  else
    Result = DAG.getNode(M680x0ISD::Wrapper, DL, PtrVT, Result);

  // With PIC, the address is actually $g + Offset.
  if (M680x0II::isGlobalRelativeToPICBase(OpFlags)) {
    Result = DAG.getNode(ISD::ADD, DL, PtrVT,
                 DAG.getNode(M680x0ISD::GlobalBaseReg, DL, PtrVT), Result);
  }

  // For globals that require a load from a stub to get the address, emit the
  // load.
  if (M680x0II::isGlobalStubReference(OpFlags)) {
    Result = DAG.getLoad(PtrVT, DL, DAG.getEntryNode(), Result,
                 MachinePointerInfo::getGOT(DAG.getMachineFunction()));
  }

  // If there was a non-zero offset that we didn't fold, create an explicit
  // addition for it.
  if (Offset != 0) {
    Result = DAG.getNode(ISD::ADD, DL, PtrVT, Result,
                 DAG.getConstant(Offset, DL, PtrVT));
  }

  return Result;
}

SDValue M680x0TargetLowering::
LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const {
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();
  return LowerGlobalAddress(GV, SDLoc(Op), Offset, DAG);
}

//===----------------------------------------------------------------------===//
// Custom Lower Jump Table
//===----------------------------------------------------------------------===//

SDValue M680x0TargetLowering::
LowerJumpTable(SDValue Op, SelectionDAG &DAG) const {
  JumpTableSDNode *JT = cast<JumpTableSDNode>(Op);

  // In PIC mode (unless we're in PCRel PIC mode) we add an offset to the
  // global base reg.
  unsigned char OpFlag = Subtarget.classifyLocalReference(nullptr);

  unsigned WrapperKind = M680x0ISD::Wrapper;
  if (M680x0II::isPCRelGlobalReference(OpFlag)) {
    WrapperKind = M680x0ISD::WrapperPC;
  }

  auto PtrVT = getPointerTy(DAG.getDataLayout());
  SDValue Result = DAG.getTargetJumpTable(JT->getIndex(), PtrVT, OpFlag);
  SDLoc DL(JT);
  Result = DAG.getNode(WrapperKind, DL, PtrVT, Result);

  // With PIC, the address is actually $g + Offset.
  if (M680x0II::isGlobalRelativeToPICBase(OpFlag)) {
    Result = DAG.getNode(ISD::ADD, DL, PtrVT,
                 DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(), PtrVT), Result);
  }

  return Result;
}

unsigned M680x0TargetLowering::
getJumpTableEncoding() const {
  return Subtarget.getJumpTableEncoding();
}

const MCExpr * M680x0TargetLowering::
LowerCustomJumpTableEntry(const MachineJumpTableInfo *MJTI,
                          const MachineBasicBlock *MBB,
                          unsigned uid, MCContext &Ctx) const{
  return MCSymbolRefExpr::create(
      MBB->getSymbol(), MCSymbolRefExpr::VK_GOTOFF, Ctx);
}

SDValue M680x0TargetLowering::
getPICJumpTableRelocBase(SDValue Table, SelectionDAG &DAG) const {
  if (getJumpTableEncoding() == MachineJumpTableInfo::EK_Custom32)
    return DAG.getNode(M680x0ISD::GlobalBaseReg, SDLoc(),
                       getPointerTy(DAG.getDataLayout()));

  // MachineJumpTableInfo::EK_LabelDifference32 entry
  return Table;
}

// NOTE This only used for MachineJumpTableInfo::EK_LabelDifference32 entries
const MCExpr *M680x0TargetLowering::
getPICJumpTableRelocBaseExpr(const MachineFunction *MF, unsigned JTI,
                             MCContext &Ctx) const {
  return MCSymbolRefExpr::create(MF->getJTISymbol(JTI, Ctx), Ctx);
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

// Return true if it is OK for this CMOV pseudo-opcode to be cascaded
// together with other CMOV pseudo-opcodes into a single basic-block with
// conditional jump around it.
static bool
isCMOVPseudo(MachineInstr &MI) {
  switch (MI.getOpcode()) {
  case M680x0::CMOV8d:
  case M680x0::CMOV16d:
  case M680x0::CMOV32r:
    return true;

  default:
    return false;
  }
}

// The CCR operand of SelectItr might be missing a kill marker
// because there were multiple uses of CCR, and ISel didn't know
// which to mark. Figure out whether SelectItr should have had a
// kill marker, and set it if it should. Returns the correct kill
// marker value.
static bool
checkAndUpdateCCRKill(MachineBasicBlock::iterator SelectItr,
                         MachineBasicBlock* BB,
                         const TargetRegisterInfo* TRI) {
  // Scan forward through BB for a use/def of CCR.
  MachineBasicBlock::iterator miI(std::next(SelectItr));
  for (MachineBasicBlock::iterator miE = BB->end(); miI != miE; ++miI) {
    const MachineInstr& mi = *miI;
    if (mi.readsRegister(M680x0::CCR))
      return false;
    if (mi.definesRegister(M680x0::CCR))
      break; // Should have kill-flag - update below.
  }

  // If we hit the end of the block, check whether CCR is live into a
  // successor.
  if (miI == BB->end()) {
    for (MachineBasicBlock::succ_iterator sItr = BB->succ_begin(),
                                          sEnd = BB->succ_end();
         sItr != sEnd; ++sItr) {
      MachineBasicBlock* succ = *sItr;
      if (succ->isLiveIn(M680x0::CCR))
        return false;
    }
  }

  // We found a def, or hit the end of the basic block and CCR wasn't live
  // out. SelectMI should have a kill flag on CCR.
  SelectItr->addRegisterKilled(M680x0::CCR, TRI);
  return true;
}

MachineBasicBlock * M680x0TargetLowering::
EmitLoweredSelect(MachineInstr &MI, MachineBasicBlock *BB) const {
  const TargetInstrInfo *TII = Subtarget.getInstrInfo();
  DebugLoc DL = MI.getDebugLoc();

  // To "insert" a SELECT_CC instruction, we actually have to insert the
  // diamond control-flow pattern.  The incoming instruction knows the
  // destination vreg to set, the condition code register to branch on, the
  // true/false values to select between, and a branch opcode to use.
  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineFunction::iterator It = ++BB->getIterator();

  //  thisMBB:
  //  ...
  //   TrueVal = ...
  //   cmpTY ccX, r1, r2
  //   bCC copy1MBB
  //   fallthrough --> copy0MBB
  MachineBasicBlock *thisMBB = BB;
  MachineFunction *F = BB->getParent();

  // This code lowers all pseudo-CMOV instructions. Generally it lowers these
  // as described above, by inserting a BB, and then making a PHI at the join
  // point to select the true and false operands of the CMOV in the PHI.
  //
  // The code also handles two different cases of multiple CMOV opcodes
  // in a row.
  //
  // Case 1:
  // In this case, there are multiple CMOVs in a row, all which are based on
  // the same condition setting (or the exact opposite condition setting).
  // In this case we can lower all the CMOVs using a single inserted BB, and
  // then make a number of PHIs at the join point to model the CMOVs. The only
  // trickiness here, is that in a case like:
  //
  // t2 = CMOV cond1 t1, f1
  // t3 = CMOV cond1 t2, f2
  //
  // when rewriting this into PHIs, we have to perform some renaming on the
  // temps since you cannot have a PHI operand refer to a PHI result earlier
  // in the same block.  The "simple" but wrong lowering would be:
  //
  // t2 = PHI t1(BB1), f1(BB2)
  // t3 = PHI t2(BB1), f2(BB2)
  //
  // but clearly t2 is not defined in BB1, so that is incorrect. The proper
  // renaming is to note that on the path through BB1, t2 is really just a
  // copy of t1, and do that renaming, properly generating:
  //
  // t2 = PHI t1(BB1), f1(BB2)
  // t3 = PHI t1(BB1), f2(BB2)
  //
  // Case 2, we lower cascaded CMOVs such as
  //
  //   (CMOV (CMOV F, T, cc1), T, cc2)
  //
  // to two successives branches.  For that, we look for another CMOV as the
  // following instruction.
  //
  // Without this, we would add a PHI between the two jumps, which ends up
  // creating a few copies all around. For instance, for
  //
  //    (sitofp (zext (fcmp une)))
  //
  // we would generate:
  //
  //         ucomiss %xmm1, %xmm0
  //         movss  <1.0f>, %xmm0
  //         movaps  %xmm0, %xmm1
  //         jne     .LBB5_2
  //         xorps   %xmm1, %xmm1
  // .LBB5_2:
  //         jp      .LBB5_4
  //         movaps  %xmm1, %xmm0
  // .LBB5_4:
  //         retq
  //
  // because this custom-inserter would have generated:
  //
  //   A
  //   | \
  //   |  B
  //   | /
  //   C
  //   | \
  //   |  D
  //   | /
  //   E
  //
  // A: X = ...; Y = ...
  // B: empty
  // C: Z = PHI [X, A], [Y, B]
  // D: empty
  // E: PHI [X, C], [Z, D]
  //
  // If we lower both CMOVs in a single step, we can instead generate:
  //
  //   A
  //   | \
  //   |  C
  //   | /|
  //   |/ |
  //   |  |
  //   |  D
  //   | /
  //   E
  //
  // A: X = ...; Y = ...
  // D: empty
  // E: PHI [X, A], [X, C], [Y, D]
  //
  // Which, in our sitofp/fcmp example, gives us something like:
  //
  //         ucomiss %xmm1, %xmm0
  //         movss  <1.0f>, %xmm0
  //         jne     .LBB5_4
  //         jp      .LBB5_4
  //         xorps   %xmm0, %xmm0
  // .LBB5_4:
  //         retq
  //
  MachineInstr *CascadedCMOV = nullptr;
  MachineInstr *LastCMOV = &MI;
  M680x0::CondCode CC = M680x0::CondCode(MI.getOperand(3).getImm());
  M680x0::CondCode OppCC = M680x0::GetOppositeBranchCondition(CC);
  MachineBasicBlock::iterator NextMIIt =
      std::next(MachineBasicBlock::iterator(MI));

  // Check for case 1, where there are multiple CMOVs with the same condition
  // first.  Of the two cases of multiple CMOV lowerings, case 1 reduces the
  // number of jumps the most.

  if (isCMOVPseudo(MI)) {
    // See if we have a string of CMOVS with the same condition.
    while (NextMIIt != BB->end() && isCMOVPseudo(*NextMIIt) &&
           (NextMIIt->getOperand(3).getImm() == CC ||
            NextMIIt->getOperand(3).getImm() == OppCC)) {
      LastCMOV = &*NextMIIt;
      ++NextMIIt;
    }
  }

  // This checks for case 2, but only do this if we didn't already find
  // case 1, as indicated by LastCMOV == MI.
  if (LastCMOV == &MI && NextMIIt != BB->end() &&
      NextMIIt->getOpcode() == MI.getOpcode() &&
      NextMIIt->getOperand(2).getReg() == MI.getOperand(2).getReg() &&
      NextMIIt->getOperand(1).getReg() == MI.getOperand(0).getReg() &&
      NextMIIt->getOperand(1).isKill()) {
    CascadedCMOV = &*NextMIIt;
  }

  MachineBasicBlock *jcc1MBB = nullptr;

  // If we have a cascaded CMOV, we lower it to two successive branches to
  // the same block.  CCR is used by both, so mark it as live in the second.
  if (CascadedCMOV) {
    jcc1MBB = F->CreateMachineBasicBlock(LLVM_BB);
    F->insert(It, jcc1MBB);
    jcc1MBB->addLiveIn(M680x0::CCR);
  }

  MachineBasicBlock *copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *sinkMBB = F->CreateMachineBasicBlock(LLVM_BB);
  F->insert(It, copy0MBB);
  F->insert(It, sinkMBB);

  // If the CCR register isn't dead in the terminator, then claim that it's
  // live into the sink and copy blocks.
  const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();

  MachineInstr *LastCCRSUser = CascadedCMOV ? CascadedCMOV : LastCMOV;
  if (!LastCCRSUser->killsRegister(M680x0::CCR) &&
      !checkAndUpdateCCRKill(LastCCRSUser, BB, TRI)) {
    copy0MBB->addLiveIn(M680x0::CCR);
    sinkMBB->addLiveIn(M680x0::CCR);
  }

  // Transfer the remainder of BB and its successor edges to sinkMBB.
  sinkMBB->splice(sinkMBB->begin(), BB,
                  std::next(MachineBasicBlock::iterator(LastCMOV)), BB->end());
  sinkMBB->transferSuccessorsAndUpdatePHIs(BB);

  // Add the true and fallthrough blocks as its successors.
  if (CascadedCMOV) {
    // The fallthrough block may be jcc1MBB, if we have a cascaded CMOV.
    BB->addSuccessor(jcc1MBB);

    // In that case, jcc1MBB will itself fallthrough the copy0MBB, and
    // jump to the sinkMBB.
    jcc1MBB->addSuccessor(copy0MBB);
    jcc1MBB->addSuccessor(sinkMBB);
  } else {
    BB->addSuccessor(copy0MBB);
  }

  // The true block target of the first (or only) branch is always sinkMBB.
  BB->addSuccessor(sinkMBB);

  // Create the conditional branch instruction.
  unsigned Opc = M680x0::GetCondBranchFromCond(CC);
  BuildMI(BB, DL, TII->get(Opc)).addMBB(sinkMBB);

  if (CascadedCMOV) {
    unsigned Opc2 = M680x0::GetCondBranchFromCond(
        (M680x0::CondCode)CascadedCMOV->getOperand(3).getImm());
    BuildMI(jcc1MBB, DL, TII->get(Opc2)).addMBB(sinkMBB);
  }

  //  copy0MBB:
  //   %FalseValue = ...
  //   # fallthrough to sinkMBB
  copy0MBB->addSuccessor(sinkMBB);

  //  sinkMBB:
  //   %Result = phi [ %FalseValue, copy0MBB ], [ %TrueValue, thisMBB ]
  //  ...
  MachineBasicBlock::iterator MIItBegin = MachineBasicBlock::iterator(MI);
  MachineBasicBlock::iterator MIItEnd =
    std::next(MachineBasicBlock::iterator(LastCMOV));
  MachineBasicBlock::iterator SinkInsertionPoint = sinkMBB->begin();
  DenseMap<unsigned, std::pair<unsigned, unsigned>> RegRewriteTable;
  MachineInstrBuilder MIB;

  // As we are creating the PHIs, we have to be careful if there is more than
  // one.  Later CMOVs may reference the results of earlier CMOVs, but later
  // PHIs have to reference the individual true/false inputs from earlier PHIs.
  // That also means that PHI construction must work forward from earlier to
  // later, and that the code must maintain a mapping from earlier PHI's
  // destination registers, and the registers that went into the PHI.

  for (MachineBasicBlock::iterator MIIt = MIItBegin; MIIt != MIItEnd; ++MIIt) {
    unsigned DestReg = MIIt->getOperand(0).getReg();
    unsigned Op1Reg = MIIt->getOperand(1).getReg();
    unsigned Op2Reg = MIIt->getOperand(2).getReg();

    // If this CMOV we are generating is the opposite condition from
    // the jump we generated, then we have to swap the operands for the
    // PHI that is going to be generated.
    if (MIIt->getOperand(3).getImm() == OppCC)
        std::swap(Op1Reg, Op2Reg);

    if (RegRewriteTable.find(Op1Reg) != RegRewriteTable.end())
      Op1Reg = RegRewriteTable[Op1Reg].first;

    if (RegRewriteTable.find(Op2Reg) != RegRewriteTable.end())
      Op2Reg = RegRewriteTable[Op2Reg].second;

    MIB = BuildMI(*sinkMBB, SinkInsertionPoint, DL,
                  TII->get(M680x0::PHI), DestReg)
          .addReg(Op1Reg).addMBB(copy0MBB)
          .addReg(Op2Reg).addMBB(thisMBB);

    // Add this PHI to the rewrite table.
    RegRewriteTable[DestReg] = std::make_pair(Op1Reg, Op2Reg);
  }

  // If we have a cascaded CMOV, the second Jcc provides the same incoming
  // value as the first Jcc (the True operand of the SELECT_CC/CMOV nodes).
  if (CascadedCMOV) {
    MIB.addReg(MI.getOperand(2).getReg()).addMBB(jcc1MBB);
    // Copy the PHI result to the register defined by the second CMOV.
    BuildMI(*sinkMBB, std::next(MachineBasicBlock::iterator(MIB.getInstr())),
            DL, TII->get(TargetOpcode::COPY),
            CascadedCMOV->getOperand(0).getReg())
        .addReg(MI.getOperand(0).getReg());
    CascadedCMOV->eraseFromParent();
  }

  // Now remove the CMOV(s).
  for (MachineBasicBlock::iterator MIIt = MIItBegin; MIIt != MIItEnd; )
    (MIIt++)->eraseFromParent();

  return sinkMBB;
}

MachineBasicBlock * M680x0TargetLowering::
EmitLoweredSegAlloca(MachineInstr &MI, MachineBasicBlock *BB) const {
  // FIXME See Target TODO.md
  llvm_unreachable("Cannot lower Segmented Stack Alloca with stack-split on");
}

MachineBasicBlock * M680x0TargetLowering::
EmitInstrWithCustomInserter(MachineInstr &MI, MachineBasicBlock *BB) const {
  switch (MI.getOpcode()) {
  default: llvm_unreachable("Unexpected instr type to insert");
  case M680x0::CMOV8d:
  case M680x0::CMOV16d:
  case M680x0::CMOV32r:
    return EmitLoweredSelect(MI, BB);
  case M680x0::SALLOCA:
    return EmitLoweredSegAlloca(MI, BB);
  }
}

SDValue M680x0TargetLowering::
LowerVASTART(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  auto PtrVT = getPointerTy(MF.getDataLayout());
  M680x0MachineFunctionInfo *FuncInfo = MF.getInfo<M680x0MachineFunctionInfo>();

  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  SDLoc DL(Op);

  // vastart just stores the address of the VarArgsFrameIndex slot into the
  // memory location argument.
  SDValue FR = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(), PtrVT);
  return DAG.getStore(Op.getOperand(0), DL, FR, Op.getOperand(1),
                      MachinePointerInfo(SV));
}


// Lower dynamic stack allocation to _alloca call for Cygwin/Mingw targets.
// Calls to _alloca are needed to probe the stack when allocating more than 4k
// bytes in one go. Touching the stack at 4K increments is necessary to ensure
// that the guard pages used by the OS virtual memory manager are allocated in
// correct sequence.
SDValue M680x0TargetLowering::
LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  bool SplitStack = MF.shouldSplitStack();

  SDLoc DL(Op);

  // Get the inputs.
  SDNode *Node = Op.getNode();
  SDValue Chain = Op.getOperand(0);
  SDValue Size  = Op.getOperand(1);
  unsigned Align = cast<ConstantSDNode>(Op.getOperand(2))->getZExtValue();
  EVT VT = Node->getValueType(0);

  // Chain the dynamic stack allocation so that it doesn't modify the stack
  // pointer when other instructions are using the stack.
  Chain = DAG.getCALLSEQ_START(Chain, DAG.getIntPtrConstant(0, DL, true), DL);

  SDValue Result;
  if (SplitStack) {
    auto &MRI = MF.getRegInfo();
    auto SPTy = getPointerTy(DAG.getDataLayout());
    auto *ARClass = getRegClassFor(SPTy);
    unsigned Vreg = MRI.createVirtualRegister(ARClass);
    Chain = DAG.getCopyToReg(Chain, DL, Vreg, Size);
    Result = DAG.getNode(M680x0ISD::SEG_ALLOCA, DL, SPTy, Chain,
                                DAG.getRegister(Vreg, SPTy));
  } else {
    auto &TLI = DAG.getTargetLoweringInfo();
    unsigned SPReg = TLI.getStackPointerRegisterToSaveRestore();
    assert(SPReg && "Target cannot require DYNAMIC_STACKALLOC expansion and"
                    " not tell us which reg is the stack pointer!");

    SDValue SP = DAG.getCopyFromReg(Chain, DL, SPReg, VT);
    Chain = SP.getValue(1);
    const TargetFrameLowering &TFI = *Subtarget.getFrameLowering();
    unsigned StackAlign = TFI.getStackAlignment();
    Result = DAG.getNode(ISD::SUB, DL, VT, SP, Size); // Value
    if (Align > StackAlign)
      Result = DAG.getNode(ISD::AND, DL, VT, Result,
                         DAG.getConstant(-(uint64_t)Align, DL, VT));
    Chain = DAG.getCopyToReg(Chain, DL, SPReg, Result); // Output chain
  }

  Chain = DAG.getCALLSEQ_END(Chain, DAG.getIntPtrConstant(0, DL, true),
                             DAG.getIntPtrConstant(0, DL, true), SDValue(), DL);

  SDValue Ops[2] = {Result, Chain};
  return DAG.getMergeValues(Ops, DL);
}

//===----------------------------------------------------------------------===//
// DAG Combine
//===----------------------------------------------------------------------===//

// SDValue M680x0TargetLowering::
// PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const {
//   SelectionDAG &DAG = DCI.DAG;
//   switch (N->getOpcode()) {
//   }
//
//   return SDValue();
// }


//===----------------------------------------------------------------------===//
// M680x0ISD Node Names
//===----------------------------------------------------------------------===//
const char *M680x0TargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case M680x0ISD::CALL:           return "M680x0ISD::CALL";
  case M680x0ISD::TAIL_CALL:      return "M680x0ISD::TAIL_CALL";
  case M680x0ISD::RET:            return "M680x0ISD::RET";
  case M680x0ISD::TC_RETURN:      return "M680x0ISD::TC_RETURN";
  case M680x0ISD::ADD:            return "M680x0ISD::ADD";
  case M680x0ISD::SUB:            return "M680x0ISD::SUB";
  case M680x0ISD::ADDX:           return "M680x0ISD::ADDX";
  case M680x0ISD::SUBX:           return "M680x0ISD::SUBX";
  case M680x0ISD::SMUL:           return "M680x0ISD::SMUL";
  case M680x0ISD::UMUL:           return "M680x0ISD::UMUL";
  case M680x0ISD::OR:             return "M680x0ISD::OR";
  case M680x0ISD::XOR:            return "M680x0ISD::XOR";
  case M680x0ISD::AND:            return "M680x0ISD::AND";
  case M680x0ISD::CMP:            return "M680x0ISD::CMP";
  case M680x0ISD::BT:             return "M680x0ISD::BT";
  case M680x0ISD::SELECT:         return "M680x0ISD::SELECT";
  case M680x0ISD::CMOV:           return "M680x0ISD::CMOV";
  case M680x0ISD::BRCOND:         return "M680x0ISD::BRCOND";
  case M680x0ISD::SETCC:          return "M680x0ISD::SETCC";
  case M680x0ISD::SETCC_CARRY:    return "M680x0ISD::SETCC_CARRY";
  case M680x0ISD::GlobalBaseReg:  return "M680x0ISD::GlobalBaseReg";
  case M680x0ISD::Wrapper:        return "M680x0ISD::Wrapper";
  case M680x0ISD::WrapperPC:      return "M680x0ISD::WrapperPC";
  case M680x0ISD::SEG_ALLOCA:     return "M680x0ISD::SEG_ALLOCA";
  default:                        return NULL;
  }
}
