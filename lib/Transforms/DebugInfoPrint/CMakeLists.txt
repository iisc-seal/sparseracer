add_llvm_loadable_module( LLVMDebugInfoPrint
  DbgInfoPrinter.cpp
  )

add_dependencies(LLVMDebugInfoPrint intrinsics_gen)
