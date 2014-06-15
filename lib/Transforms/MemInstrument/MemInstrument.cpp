
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include "FInstrument.h"
#include "LoadStoreInstrument.h"
#include "AllocFreeInstrument.h"

using namespace llvm;
namespace MemInstrument{
  static void registerMyPass(const PassManagerBuilder &,
			     PassManagerBase &PM) {
    // char MemInstrument::FInstrument::ID = 0;
    // char MemInstrument::LoadStoreInstrument::ID = 0;
    // char MemInstrument::AllocFreeInstrument::ID = 0;
    // PM.add(new MemInstrument::LoadStoreInstrument());
    PM.add(new MemInstrument::AllocFreeInstrument());
    //PM.add(new MemInstrument::FInstrument());
    
  }

  RegisterStandardPasses
  RegisterMyPass(PassManagerBuilder::EP_EnabledOnOptLevel0,
		 registerMyPass);

}
