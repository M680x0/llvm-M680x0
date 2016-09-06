//===- M680x0.td - Target definition file for the Motorola 680x0 tablegen -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is a target description file for the Motorola 680x0 family, referred
// to here as the "M680x0" architecture.
//
//===----------------------------------------------------------------------===//

// Get the target-independent interfaces which we are implementing...
include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// M680x0 processors supported.
//===----------------------------------------------------------------------===//

include "M680x0Schedule.td"

//===----------------------------------------------------------------------===//
// Register File Description
//===----------------------------------------------------------------------===//

include "M680x0RegisterInfo.td"

//===----------------------------------------------------------------------===//
// Instruction Descriptions
//===----------------------------------------------------------------------===//

include "M680x0InstrInfo.td"

def M680x0InstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Calling Conventions
//===----------------------------------------------------------------------===//

include "M680x0CallingConv.td"

//===----------------------------------------------------------------------===//
// Target
//===----------------------------------------------------------------------===//

def M680x0 : Target {
  let InstructionSet = M680x0InstrInfo;
  // let AssemblyParserVariants = [ATTAsmParserVariant, IntelAsmParserVariant];
  // let AssemblyWriters = [ATTAsmWriter, IntelAsmWriter];
}
