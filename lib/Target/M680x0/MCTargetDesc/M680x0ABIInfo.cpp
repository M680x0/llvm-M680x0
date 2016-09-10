//===---- M680x0ABIInfo.cpp - Information about CPU0 ABI's ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "M680x0ABIInfo.h"
#include "M680x0RegisterInfo.h"
#include "M680x0MCTargetDesc.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

static cl::opt<bool>
EnableM680x0S32Calls("m680x0-s32-calls", cl::Hidden,
                    cl::desc("M680x0 S32 call: use stack only to pass arguments.\
                    "), cl::init(false));

namespace {
static const MCPhysReg O32IntRegs[4] = { M680x0::D0, M680x0::D1 };
static const MCPhysReg S32IntRegs = {};
}

const ArrayRef<MCPhysReg> M680x0ABIInfo::GetByValArgRegs() const {
  if (IsO32())
    return makeArrayRef(O32IntRegs);
  if (IsS32())
    return makeArrayRef(S32IntRegs);
  llvm_unreachable("Unhandled ABI");
}

const ArrayRef<MCPhysReg> M680x0ABIInfo::GetVarArgRegs() const {
  if (IsO32())
    return makeArrayRef(O32IntRegs);
  if (IsS32())
    return makeArrayRef(S32IntRegs);
  llvm_unreachable("Unhandled ABI");
}

unsigned M680x0ABIInfo::GetCalleeAllocdArgSizeInBytes(CallingConv::ID CC) const {
  if (IsO32())
    return CC != 0;
  if (IsS32())
    return 0;
  llvm_unreachable("Unhandled ABI");
}

M680x0ABIInfo M680x0ABIInfo::computeTargetABI() {
  M680x0ABIInfo abi(ABI::Unknown);

  if (EnableM680x0S32Calls)
    abi = ABI::S32;
  else
    abi = ABI::O32;
  // Assert exactly one ABI was chosen.
  assert(abi.ThisABI != ABI::Unknown);

  return abi;
}

unsigned M680x0ABIInfo::GetStackPtr() const {
  return M680x0::SP; // alias for A7
}

unsigned M680x0ABIInfo::GetFramePtr() const {
  return M680x0::A6;
}

//FIXME check, mb all the D* registers are available
unsigned M680x0ABIInfo::GetEhDataReg(unsigned I) const {
  static const unsigned EhDataReg[] = {
    M680x0::D0, M680x0::D1
  };

  return EhDataReg[I];
}

int M680x0ABIInfo::EhDataRegSize() const {
  if (ThisABI == ABI::S32)
    return 0;
  else
    return 2;
}
