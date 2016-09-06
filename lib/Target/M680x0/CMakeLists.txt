set(LLVM_TARGET_DEFINITIONS M680x0.td)

tablegen(LLVM M680x0GenRegisterInfo.inc     -gen-register-info)
tablegen(LLVM M680x0GenInstrInfo.inc        -gen-instr-info)
tablegen(LLVM M680x0GenSubtargetInfo.inc    -gen-subtarget)
tablegen(LLVM M680x0GenMCPseudoLowering.inc -gen-pseudo-lowering)

# M680x0CommonTableGen must be defined
add_public_tablegen_target(M680x0CommonTableGen)

# M680x0CodeGen should match with LLVMBuild.txt M680x0CodeGen
add_llvm_target(M680x0CodeGen
  M680x0TargetMachine.cpp
)

# Should match with "subdirectories =  MCTargetDesc TargetInfo" in LLVMBuild.txt
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
