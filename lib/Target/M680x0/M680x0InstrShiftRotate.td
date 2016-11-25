//===------ M680x0InstrShiftRotate.td - Logical Instrs ------- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the logical instructions in the M680x0 architecture.
// Here is the current status of the file:
//
//  Machine:
//
//    SHL     [~]   ASR     [~]   LSR      [~]   SWAP     [ ]
//    ROL     [~]   ROR     [~]   ROXL     [ ]   ROXR     [ ]
//
//  Map:
//
//   [ ] - was not touched at all
//   [!] - requires extarnal stuff implemented
//   [~] - in progress but usable
//   [x] - done
//
//===----------------------------------------------------------------------===//

// $reg <- $reg op $reg
class MxSROp_DD<string mnemonic, MxType TYPE, SDNode node>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, TYPE.ROp:$opd),
           mnemonic#"."#TYPE.Prefix#"\t$opd, $dst",
           [(set TYPE.VT:$dst, (node TYPE.VT:$src, TYPE.VT:$opd))]>;

// $reg <- $reg op $imm
class MxSROp_DI<string mnemonic, MxType TYPE, SDNode node>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, !cast<Operand>("Mxi"#TYPE.Size#"imm"):$opd),
           mnemonic#"."#TYPE.Prefix#"\t$opd, $dst",
           [(set TYPE.VT:$dst, (node TYPE.VT:$src, !cast<ImmLeaf>("Mximm"#TYPE.Size#"_1to8"):$opd))]>;

multiclass MxSROp<string mnemonic, SDNode node> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {

def NAME#"8dd"  : MxSROp_DD<mnemonic, MxType8d,  node>;
def NAME#"16dd" : MxSROp_DD<mnemonic, MxType16d, node>;
def NAME#"32dd" : MxSROp_DD<mnemonic, MxType32d, node>;

def NAME#"8di"  : MxSROp_DI<mnemonic, MxType8d,  node>;
def NAME#"16di" : MxSROp_DI<mnemonic, MxType16d, node>;
def NAME#"32di" : MxSROp_DI<mnemonic, MxType32d, node>;

} // $src = $dst
} // Defs = [CCR]

} // MxBiArOp_RF

defm SHL : MxSROp<"lsl", shl>;
defm LSR : MxSROp<"lsr", srl>;
defm ASR : MxSROp<"asr", sra>;

defm ROL : MxSROp<"rol", rotl>;
defm ROR : MxSROp<"ror", rotr>;
