//===-- M680x0MachineFunctionInfo.h - Private data used for M680x0 ----*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the M680x0 specific subclass of MachineFunctionInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_M680X0MACHINEFUNCTION_H
#define LLVM_LIB_TARGET_M680X0_M680X0MACHINEFUNCTION_H

#include "llvm/ADT/StringMap.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"
#include <map>
#include <string>
#include <utility>

namespace llvm {

class M680x0MachineFunctionInfo : public MachineFunctionInfo {
  MachineFunction& MF;

  /// RestoreBasePointerOffset - Non-zero if the function has base pointer
  /// and makes call to llvm.eh.sjlj.setjmp. When non-zero, the value is a
  /// displacement from the frame pointer to a slot where the base pointer
  /// is stashed.
  signed char RestoreBasePointerOffset = 0;

  /// CalleeSavedFrameSize - Size of the callee-saved register portion of the
  /// stack frame in bytes.
  unsigned CalleeSavedFrameSize = 0;

  /// BytesToPopOnReturn - Number of bytes function pops on return (in addition
  /// to the space used by the return address).
  /// Used on windows platform for stdcall & fastcall name decoration
  unsigned BytesToPopOnReturn = 0;

  /// TailCallReturnAddrDelta - The number of bytes by which return address
  /// stack slot is moved as the result of tail call optimization.
  int TailCallReturnAddrDelta = 0;

  /// VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex = 0;

  /// HasPushSequences - Keeps track of whether this function uses sequences
  /// of pushes to pass function parameters.
  bool HasPushSequences = false;

  /// SRetReturnReg - Some subtargets require that sret lowering includes
  /// returning the value of the returned struct in a register. This field
  /// holds the virtual register into which the sret argument is passed.
  unsigned SRetReturnReg = 0;

public:
  M680x0MachineFunctionInfo() = default;
  explicit M680x0MachineFunctionInfo(MachineFunction& MF) : MF(MF) {}

  bool getRestoreBasePointer() const { return RestoreBasePointerOffset!=0; }
  void setRestoreBasePointer(const MachineFunction *MF);
  int getRestoreBasePointerOffset() const {return RestoreBasePointerOffset; }

  unsigned getCalleeSavedFrameSize() const { return CalleeSavedFrameSize; }
  void setCalleeSavedFrameSize(unsigned bytes) { CalleeSavedFrameSize = bytes; }

  unsigned getBytesToPopOnReturn() const { return BytesToPopOnReturn; }
  void setBytesToPopOnReturn (unsigned bytes) { BytesToPopOnReturn = bytes;}

  int getTCReturnAddrDelta() const { return TailCallReturnAddrDelta; }
  void setTCReturnAddrDelta(int delta) {TailCallReturnAddrDelta = delta;}

  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }

  bool getHasPushSequences() const { return HasPushSequences; }
  void setHasPushSequences(bool HasPush) { HasPushSequences = HasPush; }

  unsigned getSRetReturnReg() const { return SRetReturnReg; }
  void setSRetReturnReg(unsigned Reg) { SRetReturnReg = Reg; }

private:
  virtual void anchor();
};

} // end of namespace llvm

#endif // M680X0_MACHINE_FUNCTION_INFO_H
