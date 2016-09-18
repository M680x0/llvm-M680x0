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

void M680x0MachineFunctionInfo::anchor() { }
