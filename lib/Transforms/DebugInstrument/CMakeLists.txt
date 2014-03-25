add_llvm_loadable_module( LLVMDebugInstrument
  DebugInstrument.cpp
  )

add_dependencies(LLVMDebugInstrument intrinsics_gen)
