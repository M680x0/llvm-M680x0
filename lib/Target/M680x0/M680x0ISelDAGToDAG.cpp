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

namespace {
// For reference, the full order of operands for memory references is:
// (Operand), Displacement, Base, Index, Scale
struct M680x0ISelAddressMode {
  enum AddrType {
    ARI,     // Address Register Indirect
    ARIPI,   // Address Register Indirect with Postincrement
    ARIPD,   // Address Register Indirect with Postdecrement
    ARID,    // Address Register Indirect with Displacement
    ARII,    // Address Register Indirect with Index
    PCD,     // Program Counter Indirect with Displacement
    PCI,     // Program Counter Indirect with Index
    AL,      // Absolute
  } AM;

  enum {
    RegBase,
    FrameIndexBase
  } BaseType;

  int64_t Disp;

  // This is really a union, discriminated by BaseType!
  SDValue BaseReg;
  int BaseFrameIndex;

  SDValue IndexReg;
  unsigned Scale;

  const GlobalValue *GV;
  const Constant *CP;
  const BlockAddress *BlockAddr;
  const char *ES;
  MCSymbol *MCSym;
  int JT;
  unsigned Align;    // CP alignment.

  unsigned char SymbolFlags;  // M680x0II::MO_*

  M680x0ISelAddressMode(AddrType AT)
    : AM(AT), BaseType(RegBase),
      Disp(0), BaseFrameIndex(0), IndexReg(), Scale(1),
      GV(nullptr), CP(nullptr), BlockAddr(nullptr), ES(nullptr),
      MCSym(nullptr), JT(-1), Align(0), SymbolFlags(M680x0II::MO_NO_FLAG) {}

  bool hasSymbolicDisplacement() const {
    return GV != nullptr || CP != nullptr || ES != nullptr ||
           MCSym != nullptr || JT != -1 || BlockAddr != nullptr;
  }

  bool hasBase() const {
    return BaseType == FrameIndexBase || BaseReg.getNode() != nullptr;
  }

  bool hasBaseReg() {
    return BaseType == RegBase && BaseReg.getNode() != nullptr;
  }

  bool hasDisp() {
    return Disp != 0;
  }

  /// True if address mode type supports displacement
  bool isDispAddrType() const {
    return AM == ARII || AM == PCI || AM == ARID || AM == PCD || AM == AL;
  }

  int getDispSize() const {
    assert(isDispAddrType() && "Address mode does not support displacement");
    switch (AM) {
      default: return 0;
      case ARII:
      case PCI:
        return 8;
      // These two in the next chip generations can hold upto 32 bit
      case ARID:
      case PCD:
        return 16;
      case AL: // FIXME though really depends on code model
        return 32;
    }
  }

  bool isDisp8()   { return getDispSize() == 8;  }
  bool isDisp16()  { return getDispSize() == 16; }
  bool isDisp32()  { return getDispSize() == 32; }

  /// Return true if this addressing mode is already PC-relative.
  bool isPCRelative() const {
    if (BaseType != RegBase) return false;
    if (AM == PCI || AM == PCD) return true;
    return false;
  }

  void setBaseReg(SDValue Reg) {
    BaseType = RegBase;
    BaseReg = Reg;
  }

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  void dump() {
    dbgs() << "M680x0ISelAddressMode " << this;
    dbgs() << "\nDisp: " << Disp;
    dbgs() << ", BaseReg: ";
    if (BaseReg.getNode())
      BaseReg.getNode()->dump();
    else
      dbgs() << "null";
    dbgs() << ", BaseFI: " << BaseFrameIndex;
    dbgs() << ", IndexReg: ";
    if (IndexReg.getNode()) {
      IndexReg.getNode()->dump();
    } else {
      dbgs() << "null";
      dbgs() << ", Scale: " << Scale;
    }
    dbgs() << '\n';
  }
#endif
};
} // end anonymous namespace

namespace {

class M680x0DAGToDAGISel : public SelectionDAGISel {
public:
  explicit M680x0DAGToDAGISel(M680x0TargetMachine &TM)
      : SelectionDAGISel(TM), Subtarget(nullptr) {}

  const char *getPassName() const override {
    return "M680X0 DAG->DAG Pattern Instruction Selection";
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  /// Keep a pointer to the M680x0Subtarget around so that we can
  /// make the right decision when generating code for different targets.
  const M680x0Subtarget *Subtarget;

  // Include the pieces autogenerated from the target description.
  #include "M680x0GenDAGISel.inc"

  /// getTargetMachine - Return a reference to the TargetMachine, casted
  /// to the target-specific type.
  const M680x0TargetMachine &getTargetMachine() {
    return static_cast<const M680x0TargetMachine &>(TM);
  }

  void Select(SDNode *N) override;

  // Insert instructions to initialize the global base register in the
  // first MBB of the function.
  // HMM... do i need this?
  void initGlobalBaseReg(MachineFunction &MF);

  bool foldOffsetIntoAddress(uint64_t Offset, M680x0ISelAddressMode &AM);

  bool matchLoadInAddress(LoadSDNode *N, M680x0ISelAddressMode &AM);
  bool matchAddress(SDValue N, M680x0ISelAddressMode &AM);
  bool matchAddressBase(SDValue N, M680x0ISelAddressMode &AM);
  bool matchAddressRecursively(SDValue N, M680x0ISelAddressMode &AM,
                               unsigned Depth);
  bool matchADD(SDValue N, M680x0ISelAddressMode &AM, unsigned Depth);
  bool matchWrapper(SDValue N, M680x0ISelAddressMode &AM);

  std::pair<bool, SDNode*> selectNode(SDNode *Node);

  bool SelectARI(SDNode *Parent, SDValue N, SDValue &Base);
  bool SelectARIPI(SDNode *Parent, SDValue N, SDValue &Base);
  bool SelectARIPD(SDNode *Parent, SDValue N, SDValue &Base);
  bool SelectARID(SDNode *Parent, SDValue N, SDValue &Imm, SDValue &Base);
  bool SelectARII(SDNode *Parent, SDValue N,
                  SDValue &Imm, SDValue &Base, SDValue &Index);
  bool SelectAL(SDNode *Parent, SDValue N, SDValue &Sym);
  bool SelectPCD(SDNode *Parent, SDValue N, SDValue &Imm);

  // If Address Mode represents Frame Index store FI in Disp and
  // Displacement bit size in Base. These values are read symmetrically by
  // M680x0RegisterInfo::eliminateFrameIndex method
  inline bool getFrameIndexAddress(M680x0ISelAddressMode &AM, const SDLoc &DL,
                                   SDValue &Disp, SDValue &Base) {
    if (AM.BaseType == M680x0ISelAddressMode::FrameIndexBase) {
      Disp = getI32Imm(AM.Disp, DL);
      Base = CurDAG->getTargetFrameIndex(
          AM.BaseFrameIndex,
          TLI->getPointerTy(CurDAG->getDataLayout()));
      return true;
    }

    return false;
  }

  // Gets a symbol plus optional displacement
  inline bool getAbsoluteAddress(M680x0ISelAddressMode &AM,
                                 const SDLoc &DL,
                                 SDValue &Sym) {
    if (AM.GV) {
      Sym = CurDAG->getTargetGlobalAddress(AM.GV, SDLoc(), MVT::i32,
                                           AM.Disp, AM.SymbolFlags);
      return true;
    } else if (AM.CP) {
      Sym = CurDAG->getTargetConstantPool(AM.CP, MVT::i32,
                                          AM.Align, AM.Disp, AM.SymbolFlags);
      return true;
    } else if (AM.ES) {
      assert(!AM.Disp && "Non-zero displacement is ignored with ES.");
      Sym = CurDAG->getTargetExternalSymbol(AM.ES, MVT::i32, AM.SymbolFlags);
      return true;
    } else if (AM.MCSym) {
      assert(!AM.Disp && "Non-zero displacement is ignored with MCSym.");
      assert(AM.SymbolFlags == 0 && "oo");
      Sym = CurDAG->getMCSymbol(AM.MCSym, MVT::i32);
      return true;
    } else if (AM.JT != -1) {
      assert(!AM.Disp && "Non-zero displacement is ignored with JT.");
      Sym = CurDAG->getTargetJumpTable(AM.JT, MVT::i32, AM.SymbolFlags);
      return true;
    } else if (AM.BlockAddr) {
      Sym = CurDAG->getTargetBlockAddress(AM.BlockAddr, MVT::i32,
                                          AM.Disp, AM.SymbolFlags);
      return true;
    }

    return false;
  }

  inline bool getGOTAddress(M680x0ISelAddressMode &AM, const SDLoc &DL,
                                   SDValue &Disp, SDValue &Base) {
    if (AM.hasBaseReg() && getAbsoluteAddress(AM, DL, Disp)) {
      Base = AM.BaseReg;
      return true;
    }

    return false;
  }

  /// Return a target constant with the specified value of type i8.
  inline SDValue getI8Imm(unsigned Imm, const SDLoc &DL) {
    return CurDAG->getTargetConstant(Imm, DL, MVT::i8);
  }

  /// Return a target constant with the specified value of type i8.
  inline SDValue getI16Imm(unsigned Imm, const SDLoc &DL) {
    return CurDAG->getTargetConstant(Imm, DL, MVT::i16);
  }

  /// Return a target constant with the specified value, of type i32.
  inline SDValue getI32Imm(unsigned Imm, const SDLoc &DL) {
    return CurDAG->getTargetConstant(Imm, DL, MVT::i32);
  }

  /// Return a reference to the TargetInstrInfo, casted to the target-specific
  /// type.
  const M680x0InstrInfo *getInstrInfo() const {
    return Subtarget->getInstrInfo();
  }

  /// Return an SDNode that returns the value of the global base register.
  /// Output instructions required to initialize the global base register,
  /// if necessary.
  SDNode *getGlobalBaseReg();
};
}

bool M680x0DAGToDAGISel::
runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &static_cast<const M680x0Subtarget &>(MF.getSubtarget());
  return SelectionDAGISel::runOnMachineFunction(MF);
}

/// This pass converts a legalized DAG into a M680x0-specific DAG,
/// ready for instruction scheduling.
FunctionPass *llvm::createM680x0ISelDag(M680x0TargetMachine &TM) {
  return new M680x0DAGToDAGISel(TM);
}

/// isInt - Checks if an integer fits into the given bit width.
/// non-templated version
/// FIXME move it somewhere
LLVM_CONSTEXPR static inline bool isInt(unsigned N, int64_t x) {
  return N >= 64 || (-(INT64_C(1)<<(N-1)) <= x && x < (INT64_C(1)<<(N-1)));
}

static bool doesDispFitFI(M680x0ISelAddressMode &AM) {
  if (!AM.isDispAddrType()) return false;
  // -1 to make sure that resolved FI will fit into Disp field
  return isInt(AM.getDispSize() - 1, AM.Disp);
}

static bool doesDispFit(M680x0ISelAddressMode &AM, int64_t Val) {
  if (!AM.isDispAddrType()) return false;
  return isInt(AM.getDispSize(), Val);
}

/// Return an SDNode that returns the value of the global base register.
/// Output instructions required to initialize the global base register,
/// if necessary.
SDNode *M680x0DAGToDAGISel::getGlobalBaseReg() {
  unsigned GlobalBaseReg = getInstrInfo()->getGlobalBaseReg(MF);
  auto &DL = MF->getDataLayout();
  return CurDAG->getRegister(GlobalBaseReg, TLI->getPointerTy(DL)).getNode();
}

bool M680x0DAGToDAGISel::
foldOffsetIntoAddress(uint64_t Offset, M680x0ISelAddressMode &AM) {
  // Cannot combine ExternalSymbol displacements with integer offsets.
  if (Offset != 0 && (AM.ES || AM.MCSym))
    return false;

  int64_t Val = AM.Disp + Offset;

  if (doesDispFit(AM, Val)) {
    AM.Disp = Val;
    return true;
  }

  return false;
}

//===----------------------------------------------------------------------===//
// Matchers
//===----------------------------------------------------------------------===//

/// Helper for MatchAddress. Add the specified node to the
/// specified addressing mode without any further recursion.
bool M680x0DAGToDAGISel::
matchAddressBase(SDValue N, M680x0ISelAddressMode &AM) {
  // Is the base register already occupied?
  if (AM.BaseType != M680x0ISelAddressMode::RegBase || AM.BaseReg.getNode()) {
    // If so, check to see if the scale index register is set.
    if (!AM.IndexReg.getNode()) {
      AM.IndexReg = N;
      AM.Scale = 1;
      return true;
    }

    // Otherwise, we cannot select it.
    return false;
  }

  // Default, generate it as a register.
  AM.BaseType = M680x0ISelAddressMode::RegBase;
  AM.BaseReg = N;
  return true;
}

/// TODO
/// Have no idea how it is node with M680x0 ATM
/// Here is some description:
/// https://lists.debian.org/debian-68k/2007/11/msg00071.html
bool M680x0DAGToDAGISel::
matchLoadInAddress(LoadSDNode *N, M680x0ISelAddressMode &AM){
  // SDValue Address = N->getOperand(1);

  // load gs:0 -> GS segment register.
  // load fs:0 -> FS segment register.
  //
  // This optimization is valid because the GNU TLS model defines that
  // gs:0 (or fs:0 on M680x0-64) contains its own address.
  // For more information see http://people.redhat.com/drepper/tls.pdf
  // if (ConstantSDNode *C = dyn_cast<ConstantSDNode>(Address))
  //   if (C->getSExtValue() == 0 &&
  //       Subtarget->isTargetGlibc())
  //     switch (N->getPointerInfo().getAddrSpace()) {
  //     case 256:
  //       AM.Segment = CurDAG->getRegister(M680x0::GS, MVT::i16);
  //       return false;
  //     case 257:
  //       AM.Segment = CurDAG->getRegister(M680x0::FS, MVT::i16);
  //       return false;
  //     // Address space 258 is not handled here, because it is not used to
  //     // address TLS areas.
  //     }

  return false;
}

bool M680x0DAGToDAGISel::
matchAddressRecursively(SDValue N, M680x0ISelAddressMode &AM, unsigned Depth) {
  SDLoc DL(N);

  // DEBUG({
  //   dbgs() << "MatchAddress: ";
  //   AM.dump();
  // });

  // Limit recursion.
  if (Depth > 5)
    return matchAddressBase(N, AM);

  // If this is already a %PC relative address, we can only merge immediates
  // into it.  Instead of handling this in every case, we handle it here.
  // PC relative addressing: %PC + 16-bit displacement!
  if (AM.isPCRelative()) {
    // FIXME: JumpTable and ExternalSymbol address currently don't like
    // displacements.  It isn't very important, but this should be fixed for
    // consistency.
    // if (!(AM.ES || AM.MCSym) && AM.JT != -1)
    //   return true;

    if (ConstantSDNode *Cst = dyn_cast<ConstantSDNode>(N))
      if (foldOffsetIntoAddress(Cst->getSExtValue(), AM))
        return true;
    return false;
  }

  switch (N.getOpcode()) {
  default: break;

  case ISD::Constant: {
    uint64_t Val = cast<ConstantSDNode>(N)->getSExtValue();
    if (foldOffsetIntoAddress(Val, AM))
      return true;
    break;
  }

  case M680x0ISD::Wrapper:
  case M680x0ISD::WrapperPC:
    if (matchWrapper(N, AM))
      return true;
    break;

  case ISD::LOAD:
    if (matchLoadInAddress(cast<LoadSDNode>(N), AM))
      return true;
    break;

  case ISD::OR:
    // We want to look through a transform in InstCombine and DAGCombiner that
    // turns 'add' into 'or', so we can treat this 'or' exactly like an 'add'.
    // Example: (or (and x, 1), (shl y, 3)) --> (add (and x, 1), (shl y, 3))
    // An 'lea' can then be used to match the shift (multiply) and add:
    // and $1, %esi
    // lea (%rsi, %rdi, 8), %rax
    if (CurDAG->haveNoCommonBitsSet(N.getOperand(0), N.getOperand(1)) &&
        !matchADD(N, AM, Depth))
      return false;
    break;

  case ISD::ADD:
    if (matchADD(N, AM, Depth))
      return true;
    break;

  case ISD::FrameIndex:
    if (AM.isDispAddrType() &&
        AM.BaseType == M680x0ISelAddressMode::RegBase &&
        AM.BaseReg.getNode() == nullptr &&
        doesDispFitFI(AM)) {
      AM.BaseType = M680x0ISelAddressMode::FrameIndexBase;
      AM.BaseFrameIndex = cast<FrameIndexSDNode>(N)->getIndex();
      return true;
    }
    break;
  }

  return matchAddressBase(N, AM);
}

/// Add the specified node to the specified addressing mode, returning true if
/// it cannot be done. This just pattern matches for the addressing mode.
bool M680x0DAGToDAGISel::
matchAddress(SDValue N, M680x0ISelAddressMode &AM) {
  if (!matchAddressRecursively(N, AM, 0))
    return false;

  // Post-processing: Convert lea(,%reg,2) to lea(%reg,%reg), which has
  // a smaller encoding and avoids a scaled-index.
  // TODO make sure it is an indexed mode
  // if (AM.Scale == 2 &&
  //     AM.BaseType == M680x0ISelAddressMode::RegBase &&
  //     AM.BaseReg.getNode() == nullptr) {
  //   AM.BaseReg = AM.IndexReg;
  //   AM.Scale = 1;
  // }

  // Post-processing: Convert foo to foo(%pc), even in non-PIC mode,
  // because it has a smaller encoding.
  // TODO: Which other code models can use this?
  // FIXME this must be done only if PC* modes are currently being matched
  // if (TM.getCodeModel() == CodeModel::Small &&
  //     Subtarget->is64Bit() &&
  //     AM.Scale == 1 &&
  //     AM.BaseType == M680x0ISelAddressMode::RegBase &&
  //     AM.BaseReg.getNode() == nullptr &&
  //     AM.IndexReg.getNode() == nullptr &&
  //     AM.SymbolFlags == M680x0II::MO_NO_FLAG &&
  //     AM.hasSymbolicDisplacement())
  //   AM.BaseReg = CurDAG->getRegister(M680x0::PC, MVT::i64);

  return true;
}

bool M680x0DAGToDAGISel::
matchADD(SDValue N, M680x0ISelAddressMode &AM, unsigned Depth) {
  // Add an artificial use to this node so that we can keep track of
  // it if it gets CSE'd with a different node.
  HandleSDNode Handle(N);

  M680x0ISelAddressMode Backup = AM;
  if (matchAddressRecursively(N.getOperand(0), AM, Depth+1) &&
      matchAddressRecursively(Handle.getValue().getOperand(1), AM, Depth+1))
    return true;
  AM = Backup;

  // Try again after commuting the operands.
  if (matchAddressRecursively(Handle.getValue().getOperand(1), AM, Depth+1) &&
      matchAddressRecursively(Handle.getValue().getOperand(0), AM, Depth+1))
    return true;
  AM = Backup;

  // If we couldn't fold both operands into the address at the same time,
  // see if we can just put each operand into a register and fold at least
  // the add.
  if (AM.BaseType == M680x0ISelAddressMode::RegBase &&
      !AM.BaseReg.getNode() && !AM.IndexReg.getNode()) {
    N = Handle.getValue();
    AM.BaseReg = N.getOperand(0);
    AM.IndexReg = N.getOperand(1);
    AM.Scale = 1;
    return true;
  }

  N = Handle.getValue();
  return false;
}

/// Try to match M680x0ISD::Wrapper and M680x0ISD::WrapperPC nodes into an addressing
/// mode. These wrap things that will resolve down into a symbol reference.
/// If no match is possible, this returns true, otherwise it returns false.
bool M680x0DAGToDAGISel::
matchWrapper(SDValue N, M680x0ISelAddressMode &AM) {
  // If the addressing mode already has a symbol as the displacement, we can
  // never match another symbol.
  if (AM.hasSymbolicDisplacement())
    return false;

  SDValue N0 = N.getOperand(0);
  CodeModel::Model M = TM.getCodeModel();

  // Handle M68020 pc-relative addresses.  We check this before checking direct
  // folding because PC is preferable to non-PC accesses. Starting from x20
  // we support 32bit offsets.
  if (N.getOpcode() == M680x0ISD::WrapperPC &&
      // With original cpu we limited with 16 bit, plus if there is any offset
      // for the symbol it cannot match, since it is not poosible to represent
      // in assembly language
      ((Subtarget->isM68000() && (M == CodeModel::Small || M == CodeModel::Kernel))
       // But the newer x20 cpu can do everything we need to match up to 32 bit
       // wide references and 32 bit wide offsets
    || (Subtarget->isM68020()))) {

    llvm_unreachable("Not available right now");

    // Base and index reg must be 0 in order to use %pc as base.
    if (AM.hasBase())
      return false;

    if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(N0)) {
      M680x0ISelAddressMode Backup = AM;
      AM.GV = G->getGlobal();
      AM.SymbolFlags = G->getTargetFlags();
      if (!foldOffsetIntoAddress(G->getOffset(), AM)) {
        AM = Backup;
        return false;
      }
    } else if (ConstantPoolSDNode *CP = dyn_cast<ConstantPoolSDNode>(N0)) {
      M680x0ISelAddressMode Backup = AM;
      AM.CP = CP->getConstVal();
      AM.Align = CP->getAlignment();
      AM.SymbolFlags = CP->getTargetFlags();
      if (!foldOffsetIntoAddress(CP->getOffset(), AM)) {
        AM = Backup;
        return false;
      }
    } else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(N0)) {
      AM.ES = S->getSymbol();
      AM.SymbolFlags = S->getTargetFlags();
    } else if (auto *S = dyn_cast<MCSymbolSDNode>(N0)) {
      AM.MCSym = S->getMCSymbol();
    } else if (JumpTableSDNode *J = dyn_cast<JumpTableSDNode>(N0)) {
      AM.JT = J->getIndex();
      AM.SymbolFlags = J->getTargetFlags();
    } else if (BlockAddressSDNode *BA = dyn_cast<BlockAddressSDNode>(N0)) {
      M680x0ISelAddressMode Backup = AM;
      AM.BlockAddr = BA->getBlockAddress();
      AM.SymbolFlags = BA->getTargetFlags();
      if (!foldOffsetIntoAddress(BA->getOffset(), AM)) {
        AM = Backup;
        return false;
      }
    } else
      llvm_unreachable("Unhandled symbol reference node.");

    if (N.getOpcode() == M680x0ISD::WrapperPC)
      AM.setBaseReg(CurDAG->getRegister(M680x0::PC, MVT::i32));
    return true;
  }

  if (N.getOpcode() == M680x0ISD::Wrapper) {
    if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(N0)) {
      AM.GV = G->getGlobal();
      AM.Disp += G->getOffset();
      AM.SymbolFlags = G->getTargetFlags();
    } else if (ConstantPoolSDNode *CP = dyn_cast<ConstantPoolSDNode>(N0)) {
      AM.CP = CP->getConstVal();
      AM.Align = CP->getAlignment();
      AM.Disp += CP->getOffset();
      AM.SymbolFlags = CP->getTargetFlags();
    } else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(N0)) {
      AM.ES = S->getSymbol();
      AM.SymbolFlags = S->getTargetFlags();
    } else if (auto *S = dyn_cast<MCSymbolSDNode>(N0)) {
      AM.MCSym = S->getMCSymbol();
    } else if (JumpTableSDNode *J = dyn_cast<JumpTableSDNode>(N0)) {
      AM.JT = J->getIndex();
      AM.SymbolFlags = J->getTargetFlags();
    } else if (BlockAddressSDNode *BA = dyn_cast<BlockAddressSDNode>(N0)) {
      AM.BlockAddr = BA->getBlockAddress();
      AM.Disp += BA->getOffset();
      AM.SymbolFlags = BA->getTargetFlags();
    } else
      llvm_unreachable("Unhandled symbol reference node.");
    return true;
  }

  return false;
}

//===----------------------------------------------------------------------===//
// Selectors
//===----------------------------------------------------------------------===//

void M680x0DAGToDAGISel::Select(SDNode *Node) {
  unsigned Opcode = Node->getOpcode();
  SDLoc DL(Node);

  DEBUG(dbgs() << "Selecting: "; Node->dump(CurDAG); dbgs() << '\n');

  if (Node->isMachineOpcode()) {
    DEBUG(dbgs() << "== ";  Node->dump(CurDAG); dbgs() << '\n');
    Node->setNodeId(-1);
    return;   // Already selected.
  }

  switch(Opcode) {
  default: break;

  case M680x0ISD::GlobalBaseReg:
    ReplaceNode(Node, getGlobalBaseReg());
    return;
  }

  SelectCode(Node);
}

bool M680x0DAGToDAGISel::
SelectARI(SDNode *Parent, SDValue N, SDValue &Base) {
  DEBUG(dbgs() << "Selecting ARI: ");
  M680x0ISelAddressMode AM(M680x0ISelAddressMode::ARI);

  if (!matchAddress(N, AM)) {
    DEBUG(dbgs() << "REJECT: Match failed\n");
    return false;
  }

  // ARI does not use these
  if (AM.IndexReg.getNode() || AM.Disp != 0) {
    DEBUG(dbgs() << "REJECT: Index Reg or Disp cannot be matched by ARI\n");
    return false;
  }

  // Must be matched by AL
  if (AM.hasSymbolicDisplacement()) {
    DEBUG(dbgs() << "REJECT: Symbolic Disp cannot be matched by ARI\n");
    return false;
  }

  Base = AM.BaseReg;

  DEBUG(dbgs() << "SUCCESS\n");
  return true;;
}

bool M680x0DAGToDAGISel::
SelectARIPI(SDNode *Parent, SDValue N, SDValue &Base) {
  DEBUG(dbgs() << "Selecting ARIPI: ");
  DEBUG(dbgs() << "NOT IMPLEMENTED\n");
  return false;
}

bool M680x0DAGToDAGISel::
SelectARIPD(SDNode *Parent, SDValue N, SDValue &Base) {
  DEBUG(dbgs() << "Selecting ARIPD: ");
  DEBUG(dbgs() << "NOT IMPLEMENTED\n");
  return false;
}

bool M680x0DAGToDAGISel::
SelectARID(SDNode *Parent, SDValue N, SDValue &Disp, SDValue &Base) {
  DEBUG(dbgs() << "Selecting ARID: ");
  M680x0ISelAddressMode AM(M680x0ISelAddressMode::ARID);

  if (!matchAddress(N, AM))
    return false;

  // MVT VT = N.getSimpleValueType();
  if (AM.BaseType == M680x0ISelAddressMode::RegBase) {
    if (!AM.BaseReg.getNode()) {
      DEBUG(dbgs() << "REJECT: Could match Base Reg\n");
      return false;
    }
    // TODO Base and Index suppression is available since x20 i think
    // AM.BaseReg = CurDAG->getRegister(0, VT);
  }

  // If this is a frame index, grab it
  if (getFrameIndexAddress(AM, SDLoc(N), Disp, Base)) {
    DEBUG(dbgs() << "SUCCESS matched FI\n");
    return true;
  }

  // If this is a frame index, grab it
  if (getGOTAddress(AM, SDLoc(N), Disp, Base)) {
    DEBUG(dbgs() << "SUCCESS, matched GOT\n");
    return true;
  }

  // Give a chance to ARI
  if (AM.Disp == 0) {
    DEBUG(dbgs() << "REJECT: Should be matched by ARI\n");
    return false;
  }

  Disp = getI16Imm(AM.Disp, SDLoc(N));
  Base = AM.BaseReg;

  DEBUG(dbgs() << "SUCCESS\n");
  return true;
}

bool M680x0DAGToDAGISel::
SelectARII(SDNode *Parent, SDValue N,
                  SDValue &Imm, SDValue &Base, SDValue &Index) {
  DEBUG(dbgs() << "Selecting ARII: ");
  DEBUG(dbgs() << "NOT IMPLEMENTED\n");
  return false;
}

bool M680x0DAGToDAGISel::
SelectAL(SDNode *Parent, SDValue N, SDValue &Sym) {
  DEBUG(dbgs() << "Selecting AL: ");
  M680x0ISelAddressMode AM(M680x0ISelAddressMode::AL);

  if (!matchAddress(N, AM)) {
    DEBUG(dbgs() << "REJECT: Match failed\n");
    return false;
  }

  // AL does not use these
  if (AM.IndexReg.getNode()) {
    DEBUG(dbgs() << "REJECT: Index Reg cannot be matched by AL\n");
    return false;
  }

  if (getAbsoluteAddress(AM, SDLoc(N), Sym)) {
    DEBUG(dbgs() << "SUCCESS\n");
    return true;
  }

  return false;;
}

bool M680x0DAGToDAGISel::
SelectPCD(SDNode *Parent, SDValue N, SDValue &Disp) {
  DEBUG(dbgs() << "Selecting PCD: ");
  M680x0ISelAddressMode AM(M680x0ISelAddressMode::ARID);

  if (!matchAddress(N, AM))
    return false;

  // MVT VT = N.getSimpleValueType();
  if (AM.BaseType == M680x0ISelAddressMode::RegBase) {
    if (!AM.BaseReg.getNode()) {
      DEBUG(dbgs() << "REJECT: Could match Base Reg\n");
      return false;
    }
    // TODO Base and Index suppression is available since x20 i think
    // AM.BaseReg = CurDAG->getRegister(0, VT);
  }

  // If this is a frame index, grab it
  if (getFrameIndexAddress(AM, SDLoc(N), Disp, Base)) {
    DEBUG(dbgs() << "SUCCESS matched FI\n");
    return true;
  }

  // If this is a frame index, grab it
  if (getGOTAddress(AM, SDLoc(N), Disp, Base)) {
    DEBUG(dbgs() << "SUCCESS, matched GOT\n");
    return true;
  }

  // Give a chance to ARI
  if (AM.Disp == 0) {
    DEBUG(dbgs() << "REJECT: Should be matched by ARI\n");
    return false;
  }

  Disp = getI16Imm(AM.Disp, SDLoc(N));
  Base = AM.BaseReg;

  DEBUG(dbgs() << "SUCCESS\n");
  return true;
}
