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
#define DEBUG_TYPE "FInstrument"

#include <pthread.h>
#include "Utils.h"

using namespace llvm;
STATISTIC(FCounter, "Counts number of functions profiled");

namespace MemInstrument {
  class FInstrument : public ModulePass {

    LLVMContext *Context;
    Constant *PrintFunc;
    Constant *PrintF;
    Constant *Logger;
    std::set<std::string> skipped;
    std::set<std::string> skippedDirs;
    std::set<std::string> instrumented;
    
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

  public:
    static char ID; // Pass identification, replacement for typeid
  FInstrument() : ModulePass(ID) {ID = 0; 
      //readBlacklist();
    }
    bool runOnModule(Module &M) override;
    //void readBlacklist();
    void instrumentEntry(Function *F);
    void instrumentExits(Function *F, std::vector<BasicBlock*> exitBlocks);
    void getAnalysisUsage(AnalysisUsage &AU) const override;
  };
}
