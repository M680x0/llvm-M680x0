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
public:
  M680x0MachineFunctionInfo(MachineFunction& MF)
  : MF(MF), 
    VarArgsFrameIndex(0), 
    MaxCallFrameSize(0)
    {}

  ~M680x0MachineFunctionInfo();

  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int Index) { VarArgsFrameIndex = Index; }

private:
  virtual void anchor();

  MachineFunction& MF;

    /// VarArgsFrameIndex - FrameIndex for start of varargs area.
  int VarArgsFrameIndex;

  unsigned MaxCallFrameSize;

  /// M680x0CallEntry maps.
  StringMap<std::unique_ptr<const M680x0CallEntry>> ExternalCallEntries;
  ValueMap<const GlobalValue *, std::unique_ptr<const M680x0CallEntry>>
      GlobalCallEntries;
};

} // end of namespace llvm

#endif // M680X0_MACHINE_FUNCTION_INFO_H
