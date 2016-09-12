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

      EH_RETURN,

      // DivRem(u)
      DivRem,
      DivRemU,

      Wrapper,
    };
  }

  //===--------------------------------------------------------------------===//
  // TargetLowering Implementation
  //===--------------------------------------------------------------------===//
  class M680x0MachineFunctionInfo;
  class M680x0Subtarget;

  class M680x0TargetLowering : public TargetLowering  {
  public:
    explicit M680x0TargetLowering(const M680x0TargetMachine &TM,
                                  const M680x0Subtarget &STI);

    static const M680x0TargetLowering *create(const M680x0TargetMachine &TM,
                                              const M680x0Subtarget &STI);

    const char *getTargetNodeName(unsigned Opcode) const override;

  protected:

    /// ByValArgInfo - Byval argument information.
    struct ByValArgInfo {
      unsigned FirstIdx; // Index of the first register used.
      unsigned NumRegs;  // Number of registers used for this argument.
      unsigned Address;  // Offset of the stack area used to pass this argument.

      ByValArgInfo() : FirstIdx(0), NumRegs(0), Address(0) {}
    };

  protected:
    const M680x0Subtarget &Subtarget;
    const M680x0ABIInfo &ABI;

  private:

#if 0
    // Create a TargetConstantPool node.
    SDValue getTargetNode(ConstantPoolSDNode *N, EVT Ty, SelectionDAG &DAG,
                          unsigned Flag) const;
#endif

    /// Provide custom lowering hooks for some operations.
    ///
    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

    SDValue lowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;

    SDValue
    LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                         const SmallVectorImpl<ISD::InputArg> &Ins,
                         const SDLoc &DL, SelectionDAG &DAG,
                         SmallVectorImpl<SDValue> &InVals) const override;

    // SDValue LowerCall(CallLoweringInfo &CLI,
    //                   SmallVectorImpl<SDValue> &InVals) const override;

    SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                        const SmallVectorImpl<ISD::OutputArg> &Outs,
                        const SmallVectorImpl<SDValue> &OutVals,
                        const SDLoc &DL, SelectionDAG &DAG) const override;

  };
}

#endif // M680x0ISELLOWERING_H
