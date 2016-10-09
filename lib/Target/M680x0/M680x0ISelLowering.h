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

#include "MCTargetDesc/M680x0ABIInfo.h"
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

      CALL,

      TAIL_CALL,

      // Handle gp_rel (small data/bss sections) relocation.
      GP_REL,

      // Return
      RET,

      TC_RETURN,

      // Arithmetic operations with CCR results.
      ADD, SUB, ADDX, SUBX, SMUL,
      INC, DEC, OR, XOR, AND,

      Wrapper
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
    const M680x0ABIInfo &ABI;
  public:
    explicit M680x0TargetLowering(const M680x0TargetMachine &TM,
                                  const M680x0Subtarget &STI);

    static const M680x0TargetLowering *create(const M680x0TargetMachine &TM,
                                              const M680x0Subtarget &STI);

    const char *getTargetNodeName(unsigned Opcode) const override;

    /// Return the value type to use for ISD::SETCC.
    EVT getSetCCResultType(const DataLayout &DL, LLVMContext &Context,
                           EVT VT) const override;

    /// Provide custom lowering hooks for some operations.
    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  private:
    unsigned GetAlignedArgumentStackSize(unsigned StackSize,
                                         SelectionDAG &DAG) const;

    SDValue LowerMemArgument(SDValue Chain, CallingConv::ID CallConv,
                             const SmallVectorImpl<ISD::InputArg> &ArgInfo,
                             const SDLoc &dl, SelectionDAG &DAG,
                             const CCValAssign &VA, MachineFrameInfo &MFI,
                             unsigned i) const;

    /// LowerFormalArguments - transform physical registers into virtual
    /// registers and generate load operations for arguments places on the
    /// stack.
    SDValue
    LowerFormalArguments(SDValue Chain, CallingConv::ID CCID, bool isVarArg,
                         const SmallVectorImpl<ISD::InputArg> &Ins,
                         const SDLoc &DL, SelectionDAG &DAG,
                         SmallVectorImpl<SDValue> &InVals) const override;

    // SDValue LowerCall(CallLoweringInfo &CLI,
    //                   SmallVectorImpl<SDValue> &InVals) const override;

    SDValue LowerReturn(SDValue Chain, CallingConv::ID CCID, bool IsVarArg,
                        const SmallVectorImpl<ISD::OutputArg> &Outs,
                        const SmallVectorImpl<SDValue> &OutVals,
                        const SDLoc &DL, SelectionDAG &DAG) const override;

  };
}

#endif // M680x0ISELLOWERING_H
