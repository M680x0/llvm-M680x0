//===-- M680x0Schedule.td - M680x0 Scheduling Definitions ----- tablegen --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// This is a very general M680x0 Scheduling Model and best suited for the very
// first M68000 CPU, other model must override these characteristics
class M680x0SchedModel : SchedMachineModel {
  let LoadLatency = 4;  // Word (Rn)
  let HighLatency = 16; // Long ABS
  let PostRAScheduler = 0;
  let CompleteModel = 0;
}

def GenericM680x0Model : M680x0SchedModel;
