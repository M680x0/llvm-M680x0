//== M680x0RegisterInfo.td - Target definition file for the M680x0 tablegen ==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the M680x0 Register file, defining the registers aliases
// between the registers, and the register classes built out of the registers.
//
//===----------------------------------------------------------------------===//

class M680x0Reg<string n, bits<16> enc, list<Register> subregs = []>
    : Register<n> {
  let Namespace = "M680x0";
  let HWEncoding = enc;
  let SubRegs    = subregs;
}

// Subregister indices.
let Namespace = "M680x0" in {
  def SubRegIndex8Lo : SubRegIndex<8, 0>;
}

// Data Registers
class M680x0DataReg<string n, bits<16> enc> : M680x0Reg<n, enc>;

foreach  Index = 0-7 in {
  def D#Index : M680x0DataReg<"d"#Index, #Index>;
}

// Address Registers
class M680x0AddressReg<string n, bits<16> enc> : M680x0Reg<n, enc>;

foreach  Index = 0-7 in {
  def A#Index : M680x0AddressReg<"a"#Index, #Index>;
}

// Alias Registers
class M680x0AliasReg<string n, M680x0Reg reg>
    : M680x0Reg<n, reg.HWEncoding, reg.SubRegs> {
  let Aliases = [reg];
}

def FP  : M680x0AliasReg<"fp",  A6>; // FIXME check if this is correct register

def SP  : M680x0AliasReg<"sp",  A7>;
def USP : M680x0AliasReg<"usp", A7>;
def SSP : M680x0AliasReg<"ssp", A7>;
def ISP : M680x0AliasReg<"isp", A7>;

// Pseudo Registers
class M680x0PseudoReg<string n, list<Register> subregs = []>
    : M680x0Reg<n, 0, subregs>;

def CCR : M680x0PseudoReg<"ccr">;

let SubRegIndices = [SubRegIndex8Lo] in {
  def SR : M680x0PseudoReg<"sr", [CCR]>;
}

def PC : M680x0PseudoReg<"pc">;

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

class M680x0RegClass<list<ValueType> regTypes, int alignment, dag regList>
    : RegisterClass<"M680x0", regTypes, alignment, regList>;

// Data Registers
def DR8  : M680x0RegClass<[i8],  16, (sequence "D%u", 0, 7)>;
def DR16 : M680x0RegClass<[i16], 16, (sequence "D%u", 0, 7)>;
def DR32 : M680x0RegClass<[i32], 32, (sequence "D%u", 0, 7)>;

// Address Registers
// A7 is not part of address class due to its strict use as Stack Pointer
def AR16 : M680x0RegClass<[i16], 16, (sequence "A%u", 0, 6)>;
def AR32 : M680x0RegClass<[i32], 32, (sequence "A%u", 0, 6)>;

// It is possible to use A* registers as data source, but they are limited to
// 16 and 32 bit operations
def SRS16 : M680x0RegClass<[i16], 16, (add DR16, AR16)>;
def SRS32 : M680x0RegClass<[i32], 32, (add DR32, AR32)>;

// It is possible to use A* registers as data source, but they are limited to
// 16 and 32 bit operations. When an address register used as the destination
// operand, the entire register is affected regardless of the operation size.
// If the operation size is word, any other operands are sign-extended to 32bits
// before the operation is performed.
// TODO find a way to control sign-extension for load into AR16
def DST16 : M680x0RegClass<[i16], 16, (add DR16, AR16)>;
def DST32 : M680x0RegClass<[i32], 32, (add DR32, AR32)>;

// Index Register Classes
def IR16 : M680x0RegClass<[i16], 16, (add DR16, AR16)>;
def IR32 : M680x0RegClass<[i32], 32, (add DR32, AR32)>;

// Control Registers
let CopyCost = -1, isAllocatable = 0 in {
  def CR8  : M680x0RegClass<[i8],  16, (add CCR)>;
  def CR16 : M680x0RegClass<[i16], 16, (add SR)>;
  def CR32 : M680x0RegClass<[i32], 32, (add PC)>;
  def CSP  : M680x0RegClass<[i32], 32, (add SP)>;
}
