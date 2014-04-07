#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LLVMContext.h"
//#include "llvm/Target/TargetData.h"                                                                     
#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/ValueTracking.h"

#include <stdint.h>

#include "Utils.h"
using namespace llvm;

namespace MemInstrument {

  class LoadStoreInstrument : public ModulePass {

    Constant *MopFn;
    Type *IntptrTy;
    LLVMContext *Context;
    const DataLayout *DL;

    std::set<std::string> InstrumentedTypes = {
      //      "product",                                                                                  
      "nsFrameSelection",
      "nsGenericHTMLElement",
      "nsHTMLEditor",
      "nsHTMLDocument",
      "nsWindow",
      "PresShell"
      "nsGlobalWindowObserver"
    };
    
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  public:
    static char ID;
  LoadStoreInstrument() : ModulePass(ID) {ID = 0;}
    virtual bool runOnModule(Module &M);
    void Instrument(BasicBlock::iterator &BI, bool isStore);
    virtual bool runOnBasicBlock(Function::iterator &BB);
  
  };

}
