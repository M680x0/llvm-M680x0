//===-- M680x0ISelDAGToDAG.cpp - A Dag to Dag Inst Selector for M680x0 --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the M680X0 target.
//
//===----------------------------------------------------------------------===//

#include "M680x0ISelDAGToDAG.h"
#include "M680x0.h"

#include "M680x0MachineFunction.h"
#include "M680x0RegisterInfo.h"
#include "M680x0TargetMachine.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;

#define DEBUG_TYPE "m680x0-isel"

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// M680x0DAGToDAGISel - M680x0 specific code to select M680x0 machine
// instructions for SelectionDAG operations.
//===----------------------------------------------------------------------===//

bool M680x0DAGToDAGISel::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const M680x0Subtarget &>(MF.getSubtarget());
  return SelectionDAGISel::runOnMachineFunction(MF);
}

bool M680x0DAGToDAGISel::selectARIPI(SDNode *Parent, SDValue N, SDValue &Base) {
    return false;
}

std::pair<bool, SDNode*> M680x0DAGToDAGISel::selectNode(SDNode *Node) {
  // unsigned Opcode = Node->getOpcode();
  // SDLoc DL(Node);
  //
  // ///
  // // Instruction Selection not handled by the auto-generated
  // // tablegen selection should be handled here.
  // ///
  // SDNode *Result;
  //
  // ///
  // // Instruction Selection not handled by the auto-generated
  // // tablegen selection should be handled here.
  // ///
  // EVT NodeTy = Node->getValueType(0);
  // unsigned MultOpc;
  //
  // switch(Opcode) {
  // default: break;
  //
  // }
  //
  return std::make_pair(false, nullptr);
}

/// Select instructions not customized! Used for
/// expanded, promoted and normal instructions
void M680x0DAGToDAGISel::Select(SDNode *Node) {
  unsigned Opcode = Node->getOpcode();

  DEBUG(
    dbgs() << "M680x0 Select: ";
    Node->dump(CurDAG);
    dbgs() << "\n");

  // If we have a custom node, we already have selected!
  if (Node->isMachineOpcode()) {
    DEBUG(dbgs() << "== "; Node->dump(CurDAG); dbgs() << "\n");
    Node->setNodeId(-1);
    return;
  }

  // See if subclasses can handle this node.
  std::pair<bool, SDNode*> Ret = selectNode(Node);

  if (Ret.first)
  {
    ReplaceNode(Node, Ret.second);
    return;
  }

  switch(Opcode) {
  default: break;

#if 0
//#ifndef NDEBUG
  case ISD::LOAD:
  case ISD::STORE:
    assert((Subtarget->systemSupportsUnalignedAccess() ||
            cast<MemSDNode>(Node)->getMemoryVT().getSizeInBits() / 8 <=
            cast<MemSDNode>(Node)->getAlignment()) &&
           "Unexpected unaligned loads/stores.");
    break;
#endif
  }

  DEBUG(
    dbgs() << "M680x0 SelectCode: ";
    Node->dump();
    dbgs() << "\n");

  SelectCode(Node);
}

/// This pass converts a legalized DAG into a M680x0-specific DAG,
/// ready for instruction scheduling.
FunctionPass *llvm::createM680x0ISelDag(M680x0TargetMachine &TM) {
  return new M680x0DAGToDAGISel(TM);
}
