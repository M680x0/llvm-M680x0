//===-- M680x0MachineFunctionInfo.cpp - Private data used for M680x0 ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "M680x0MachineFunction.h"

#include "M680x0InstrInfo.h"
#include "M680x0Subtarget.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

bool FixGlobalBaseReg;

M680x0CallEntry::M680x0CallEntry(StringRef N)
    : CallEntryPseudoSourceValue(PSVKind::GOT) {
#ifndef NDEBUG
  Name = N;
  Val = nullptr;
#endif
}

M680x0CallEntry::M680x0CallEntry(const GlobalValue *V)
    : CallEntryPseudoSourceValue(PSVKind::GOT) {
#ifndef NDEBUG
  Val = V;
#endif
}

void M680x0CallEntry::printCustom(raw_ostream &O) const {
  O << "M680x0CallEntry: ";
#ifndef NDEBUG
  if (Val)
    O << Val->getName();
  else
    O << Name;
#endif
}

M680x0MachineFunctionInfo::~M680x0MachineFunctionInfo() {}

void M680x0MachineFunctionInfo::anchor() { }
