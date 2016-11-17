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

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// X86 Subtarget features
//===----------------------------------------------------------------------===//

def FeatureISAx00
  : SubtargetFeature<"x00", "IsM68000", "true", "Is M68000 ISA supported">;

def FeatureISAx10
  : SubtargetFeature<"x10", "IsM68010", "true", "Is M68010 ISA supported",
    [ FeatureISAx00 ]>;

def FeatureISAx20
  : SubtargetFeature<"x20", "IsM68020", "true", "Is M68020 ISA supported",
    [ FeatureISAx10 ]>;

def FeatureISAx30
  : SubtargetFeature<"x30", "IsM68030", "true", "Is M68030 ISA supported",
    [ FeatureISAx20 ]>;

def FeatureISAx40
  : SubtargetFeature<"x40", "IsM68040", "true", "Is M68040 ISA supported",
    [ FeatureISAx30 ]>;

//===----------------------------------------------------------------------===//
// M680x0 processors supported.
//===----------------------------------------------------------------------===//

include "M680x0Schedule.td"

class Proc<string Name, list<SubtargetFeature> Features>
    : ProcessorModel<Name, GenericM680x0Model, Features>;

def : Proc<"generic", [ FeatureISAx20 ]>;
def : Proc<"M68000",  [ FeatureISAx00 ]>;
def : Proc<"M68010",  [ FeatureISAx10 ]>;
def : Proc<"M68020",  [ FeatureISAx20 ]>;
def : Proc<"M68030",  [ FeatureISAx30 ]>;
def : Proc<"M68040",  [ FeatureISAx40 ]>;

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
