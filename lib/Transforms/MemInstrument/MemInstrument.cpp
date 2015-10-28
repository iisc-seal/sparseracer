#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/CommandLine.h"

#include "FInstrument.h"
#include "LoadStoreInstrument.h"
#include "AllocFreeInstrument.h"
#include "RunnableInstrument.h"

using namespace llvm;

static cl::opt<bool> FInstr("finstrument", cl::desc("Instrument function entry and exit."));
static cl::opt<bool> AllocFree("allocfree", cl::desc("Instrument allocs and frees."));
static cl::opt<bool> LoadStore("loadstore", cl::desc("Instrument loads and stores."));

namespace MemInstrument{
  static void registerMyPass(const PassManagerBuilder &,
			     PassManagerBase &PM) {
    // char MemInstrument::FInstrument::ID = 0;
    // char MemInstrument::LoadStoreInstrument::ID = 0;
    // char MemInstrument::AllocFreeInstrument::ID = 0;
    if(LoadStore){
      llvm::outs() << "Registered LoadStoreInstrument \n";
      PM.add(new MemInstrument::LoadStoreInstrument());
    }
    if(AllocFree){
      llvm::outs() << "Registered AllocFreeInstrument \n";
      PM.add(new MemInstrument::AllocFreeInstrument());
    }
    if(FInstr){
      llvm::outs() << "Registered FInstrument \n";
      PM.add(new MemInstrument::FInstrument());
    }

    // Default, if no flag is passed, then enable everything
    if(!LoadStore && !AllocFree && !FInstr){
      llvm::outs() << "Tracking access, allocs, frees, function entry/exit \n";
      PM.add(new MemInstrument::LoadStoreInstrument());
      PM.add(new MemInstrument::AllocFreeInstrument());
      PM.add(new MemInstrument::FInstrument());
    }
    //    PM.add(new MemInstrument::RunnableInstrument());
    
  }

  static RegisterStandardPasses
  //RegisterMyPass(PassManagerBuilder::EP_EnabledOnOptLevel0, registerMyPass);
  RegisterMyPass(PassManagerBuilder::EP_OptimizerLast, registerMyPass);

}
