//===-- M680x0ISelLowering.h - M680x0 DAG Lowering Interface ----*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_M680X0_M680X0ISELLOWERING_H
#define LLVM_LIB_TARGET_M680X0_M680X0ISELLOWERING_H

#include "M680x0.h"

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/Function.h"
#include "llvm/Target/TargetLowering.h"
#include <deque>

namespace llvm {
  namespace M680x0ISD {
    // M680x0 Specific DAG nodes
    enum NodeType {
      // Start the numbering from where ISD NodeType finishes.
      FIRST_NUMBER = ISD::BUILTIN_OP_END,

      CALL, RET, TAIL_CALL, TC_RETURN,

      /// M680x0 compare and logical compare instructions. Subtracts the source
      /// operand from the destination data register and sets the condition
      /// codes according to the result. Immediate always goes first.
      CMP,

      /// M680x0 bit-test instructions.
      BT,

      /// M680x0 Select
      SELECT,

      /// M680x0 SetCC. Operand 0 is condition code, and operand 1 is the CCR
      /// operand, usually produced by a CMP instruction.
      SETCC,

      // Same as SETCC except it's materialized with a subx and the value is all
      // one's or all zero's.
      SETCC_CARRY,  // R = carry_bit ? ~0 : 0

      /// M680x0 conditional moves. Operand 0 and operand 1 are the two values
      /// to select from. Operand 2 is the condition code, and operand 3 is the
      /// flag operand produced by a CMP or TEST instruction. It also writes a
      /// flag result.
      CMOV,

      /// M680x0 conditional branches. Operand 0 is the chain operand, operand 1
      /// is the block to branch if condition is true, operand 2 is the
      /// condition code, and operand 3 is the flag operand produced by a CMP
      /// or TEST instruction.
      BRCOND,

      // Arithmetic operations with CCR results.
      ADD, SUB, ADDX, SUBX,
      SMUL, UMUL,
      OR, XOR, AND,

      GlobalBaseReg,

      /// A wrapper node for TargetConstantPool,
      /// TargetExternalSymbol, and TargetGlobalAddress.
      Wrapper,

      /// Special wrapper used under M680x0 PIC mode for PC
      /// relative displacements.
      WrapperPC,
    };
  }

  /// Define some predicates that are used for node matching.
  namespace M680x0 {

    /// Determines whether the callee is required to pop its
    /// own arguments. Callee pop is necessary to support tail calls.
    bool isCalleePop(CallingConv::ID CallingConv,
                     bool IsVarArg, bool GuaranteeTCO);

  } // end namespace M680x0

  //===--------------------------------------------------------------------===//
  // TargetLowering Implementation
  //===--------------------------------------------------------------------===//
  class M680x0MachineFunctionInfo;
  class M680x0Subtarget;

  class M680x0TargetLowering : public TargetLowering  {
    const M680x0Subtarget &Subtarget;

  public:
    explicit M680x0TargetLowering(const M680x0TargetMachine &TM,
                                  const M680x0Subtarget &STI);

    static const M680x0TargetLowering *create(const M680x0TargetMachine &TM,
                                              const M680x0Subtarget &STI);

    const char *getTargetNodeName(unsigned Opcode) const override;

    /// Return the value type to use for ISD::SETCC.
    EVT getSetCCResultType(const DataLayout &DL, LLVMContext &Context,
                           EVT VT) const override;

    /// EVT is not used in-tree, but is used by out-of-tree target.
    virtual MVT getScalarShiftAmountTy(const DataLayout &, EVT) const override;

    /// Provide custom lowering hooks for some operations.
    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

    // Replace the results of node with an illegal result type with new values
    // built out of custom code.
    // void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue>&Results,
    //                         SelectionDAG &DAG) const override;

    SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

    MachineBasicBlock *
    EmitInstrWithCustomInserter(MachineInstr &MI,
                                MachineBasicBlock *MBB) const override;

  private:
    unsigned GetAlignedArgumentStackSize(unsigned StackSize,
                                         SelectionDAG &DAG) const;

    SDValue getReturnAddressFrameIndex(SelectionDAG &DAG) const;

    /// Emit a load of return address if tail call
    /// optimization is performed and it is required.
    SDValue EmitTailCallLoadRetAddr(SelectionDAG &DAG, SDValue &OutRetAddr,
                                    SDValue Chain, bool IsTailCall,
                                    int FPDiff, const SDLoc &DL) const;

    /// Emit a store of the return address if tail call
    /// optimization is performed and it is required (FPDiff!=0).
    SDValue EmitTailCallStoreRetAddr(SelectionDAG &DAG, MachineFunction &MF,
                                     SDValue Chain, SDValue RetAddrFrIdx,
                                     EVT PtrVT, unsigned SlotSize, int FPDiff,
                                     const SDLoc &DL) const;

    SDValue LowerMemArgument(SDValue Chain, CallingConv::ID CallConv,
                             const SmallVectorImpl<ISD::InputArg> &ArgInfo,
                             const SDLoc &DL, SelectionDAG &DAG,
                             const CCValAssign &VA, MachineFrameInfo &MFI,
                             unsigned i) const;

    SDValue LowerMemOpCallTo(SDValue Chain, SDValue StackPtr, SDValue Arg,
                             const SDLoc &DL, SelectionDAG &DAG,
                             const CCValAssign &VA,
                             ISD::ArgFlagsTy Flags) const;

    SDValue LowerMUL(SDValue &N, SelectionDAG &DAG) const;
    SDValue LowerXALUO(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerToBT(SDValue And, ISD::CondCode CC, const SDLoc &DL,
                      SelectionDAG &DAG) const;
    SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSETCCE(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSELECT(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerBRCOND(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerADDC_ADDE_SUBC_SUBE(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerExternalSymbol(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerGlobalAddress(const GlobalValue *GV, const SDLoc &DL,
                               int64_t Offset, SelectionDAG &DAG) const;
    SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;

    SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                            CallingConv::ID CallConv, bool isVarArg,
                            const SmallVectorImpl<ISD::InputArg> &Ins,
                            const SDLoc &DL, SelectionDAG &DAG,
                            SmallVectorImpl<SDValue> &InVals) const;

    /// LowerFormalArguments - transform physical registers into virtual
    /// registers and generate load operations for arguments places on the stack.
    SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CCID,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

    SDValue LowerCall(CallLoweringInfo &CLI,
                      SmallVectorImpl<SDValue> &InVals) const override;

    /// Lower the result values of a call into the
    /// appropriate copies out of appropriate physical registers.
    SDValue LowerReturn(SDValue Chain, CallingConv::ID CCID, bool IsVarArg,
                        const SmallVectorImpl<ISD::OutputArg> &Outs,
                        const SmallVectorImpl<SDValue> &OutVals,
                        const SDLoc &DL, SelectionDAG &DAG) const override;

    MachineBasicBlock *EmitLoweredSelect(MachineInstr &I,
                                         MachineBasicBlock *BB) const;

    /// Emit nodes that will be selected as "test Op0,Op0", or something
    /// equivalent, for use with the given M680x0 condition code.
    SDValue EmitTest(SDValue Op0, unsigned M680x0CC, const SDLoc &dl,
                     SelectionDAG &DAG) const;

    /// Emit nodes that will be selected as "cmp Op0,Op1", or something
    /// equivalent, for use with the given M680x0 condition code.
    SDValue EmitCmp(SDValue Op0, SDValue Op1, unsigned M680x0CC, const SDLoc &dl,
                    SelectionDAG &DAG) const;

    /// Check whether the call is eligible for tail call optimization. Targets
    /// that want to do tail call optimization should implement this function.
    bool IsEligibleForTailCallOptimization(SDValue Callee,
                                           CallingConv::ID CalleeCC,
                                           bool isVarArg,
                                           bool isCalleeStructRet,
                                           bool isCallerStructRet,
                                           Type *RetTy,
                                    const SmallVectorImpl<ISD::OutputArg> &Outs,
                                    const SmallVectorImpl<SDValue> &OutVals,
                                    const SmallVectorImpl<ISD::InputArg> &Ins,
                                           SelectionDAG& DAG) const;
  };
}

#endif // M680x0ISELLOWERING_H
