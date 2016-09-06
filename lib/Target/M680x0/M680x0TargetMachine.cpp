//===-- M680x0TargetMachine.cpp - Define TargetMachine for M680x0 ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about M680x0 target spec.
//
//===----------------------------------------------------------------------===//

#include "M680x0TargetMachine.h"
#include "M680x0.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

#define DEBUG_TYPE "M680x0"

extern "C" void LLVMInitializeM680x0Target() {
}
