add_llvm_loadable_module( LLVMMemInstrument  
  Utils.cpp 
  LoadStoreInstrument.cpp
  FInstrument.cpp
  RunnableInstrument.cpp
  AllocFreeInstrument.cpp
  MemInstrument.cpp
  )

add_dependencies(LLVMMemInstrument intrinsics_gen)
