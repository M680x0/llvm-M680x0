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

/// \brief The class represents a GOT entry resolved by lazy-binding.
class M680x0CallEntry : public CallEntryPseudoSourceValue {
public:
  explicit M680x0CallEntry(StringRef N);
  explicit M680x0CallEntry(const GlobalValue *V);

private:
  void printCustom(raw_ostream &O) const override;
#ifndef NDEBUG
  std::string Name;
  const GlobalValue *Val;
#endif
};

class M680x0MachineFunctionInfo : public MachineFunctionInfo {
  MachineFunction& MF;

  /// VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex = 0;

  // FIXME add description
  unsigned MaxCallFrameSize = 0;

  /// SRetReturnReg - Some subtargets require that sret lowering includes
  /// returning the value of the returned struct in a register. This field
  /// holds the virtual register into which the sret argument is passed.
  unsigned SRetReturnReg = 0;

  /// True if function has a byval argument.
  bool HasByvalArg = false;

  /// Size of incoming argument area.
  unsigned IncomingArgSize = 0;

  /// CallsEhReturn - Whether the function calls llvm.eh.return.
  bool CallsEhReturn = false;

  /// CallsEhDwarf - Whether the function calls llvm.eh.dwarf.
  bool CallsEhDwarf = false;

  /// Frame objects for spilling eh data registers.
  int EhDataRegFI[2]; // FIXME not sure if this is correct

private:
  virtual void anchor();

  /// M680x0CallEntry maps.
  StringMap<std::unique_ptr<const M680x0CallEntry>> ExternalCallEntries;
  ValueMap<const GlobalValue *, std::unique_ptr<const M680x0CallEntry>>
      GlobalCallEntries;

public:
  M680x0MachineFunctionInfo() = default;
  explicit M680x0MachineFunctionInfo(MachineFunction& MF) : MF(MF) {}

  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }

  unsigned getSRetReturnReg() const { return SRetReturnReg; }
  void setSRetReturnReg(unsigned Reg) { SRetReturnReg = Reg; }

  bool hasByvalArg() const { return HasByvalArg; }

  void setFormalArgInfo(unsigned Size, bool HasByval) {
    IncomingArgSize = Size;
    HasByvalArg = HasByval;
  }
};

} // end of namespace llvm

#endif // M680X0_MACHINE_FUNCTION_INFO_H
