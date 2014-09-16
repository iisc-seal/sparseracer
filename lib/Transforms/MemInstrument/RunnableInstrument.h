#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/raw_ostream.h"
//#define DEBUG_TYPE "RunnableInstrument"

#include <pthread.h>
#include "Utils.h"

using namespace llvm;
//STATISTIC(FCounter, "Counts number of functions profiled");

namespace MemInstrument {
  class RunnableInstrument : public ModulePass {
    Type *IntptrTy;
    LLVMContext *Context;
    Constant *Logger;

  public:
    static char ID; // Pass identification, replacement for typeid
  RunnableInstrument() : ModulePass(ID) {ID = 0;}
    bool runOnModule(Module &M) override;
    void instrumentEntry(Function *F, std::string demangledName);
    void instrumentExits(Function *F, std::vector<BasicBlock*> exitBlocks, std::string demangledName);
    void getAnalysisUsage(AnalysisUsage &AU) const override;
  };
}
