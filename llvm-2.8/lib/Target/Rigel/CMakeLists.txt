set(LLVM_TARGET_DEFINITIONS Rigel.td)

tablegen(RigelGenRegisterInfo.h.inc -gen-register-desc-header)
tablegen(RigelGenRegisterNames.inc -gen-register-enums)
tablegen(RigelGenRegisterInfo.inc -gen-register-desc)
tablegen(RigelGenInstrNames.inc -gen-instr-enums)
tablegen(RigelGenInstrInfo.inc -gen-instr-desc)
tablegen(RigelGenAsmWriter.inc -gen-asm-writer)
tablegen(RigelGenDAGISel.inc -gen-dag-isel)
tablegen(RigelGenCallingConv.inc -gen-callingconv)
tablegen(RigelGenSubtarget.inc -gen-subtarget)

add_llvm_target(RigelCodeGen
	RigelExpandPseudoInsts.cpp
  RigelInstrInfo.cpp
  RigelISelDAGToDAG.cpp
  RigelISelLowering.cpp
  RigelMCAsmInfo.cpp
  RigelRegisterInfo.cpp
  RigelSubtarget.cpp
  RigelTargetMachine.cpp
  RigelTargetObjectFile.cpp
  RigelSelectionDAGInfo.cpp
  )

target_link_libraries (LLVMRigelCodeGen LLVMSelectionDAG)
