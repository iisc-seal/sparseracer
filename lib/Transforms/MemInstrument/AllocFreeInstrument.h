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
#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"

#include <stdint.h>

#include "Utils.h"

using namespace llvm;

namespace MemInstrument {
  class AllocFreeInstrument : public ModulePass {

    Constant *MDallocFn;
    Constant *MAllocFn;

    // Consider storing in a sorted vector once the code is stable                                       
    std::set<std::string> allocFunctions = {
      "_Znwm",  // operator new(unsigned long)
      "_Znam",  // operator new[](unsigned long)
      "malloc", // void *malloc(size_t size);
      "moz_malloc", // void* moz_malloc(size_t size)
      "moz_xmalloc" // The |moz_x| versions will never return a NULL pointer
    };
    
    std::set<std::string> freeFunctions = {
      "_ZdlPv", // operator delete(void*)
      "_ZdaPv", // operator delete[](void*)
      "free",
      "moz_free"
    };

    std::set<std::string> whiteList = {
      "nsHTMLEditor::ContentAppended",
      "nsHTMLEditor::ContentInserted",
      "nsHTMLEditor::ResetRootElementAndEventTarget",
      "nsFrameSelection::SetAncestorLimiter",
      "nsGenericHTMLElement::Focus",
      "nsINode::AppendChild",
      "nsINode::RemoveChild",
      "nsEditor::InitializeSelection"
    };

    Type *IntptrTy;
    LLVMContext *Context;
    const DataLayout *DL;
    
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  public:
    static char ID;
  AllocFreeInstrument() : ModulePass(ID) {ID = 0;}
    virtual bool runOnModule(Module &M);
    void InstrumentDealloc(BasicBlock::iterator &BI, std::string fName);
    void InstrumentAlloc(BitCastInst* Succ, CallInst *Original, std::string fName);
    virtual bool runOnBasicBlock(Function::iterator &BB);
    std::set<std::string> getAllocFunctions(){
      return allocFunctions;
    }
    std::set<std::string> getFreeFunctions(){
      return freeFunctions;
    }
  };
}
