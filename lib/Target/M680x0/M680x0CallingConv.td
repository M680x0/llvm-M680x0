//===-- M680X0CallingConv.td - Calling Conventions M680X0 tablegen --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for the M680x0 architectures.
//
//===----------------------------------------------------------------------===//

class CCIfSubtarget<string F, CCAction A>
    : CCIf<!strconcat("static_cast<const M680X0Subtarget&>"
                       "(State.getMachineFunction().getSubtarget()).", F), A>;

// TODO Define Stack-based and Register based Classic Mac CC
// TODO Define C call 
// TODO Check what GCC is using by default

// FIXME this is a stub CSR, O32 is register based CC
def CSR_O32 : CalleeSavedRegs<(add PC, FP)>;

