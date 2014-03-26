//===- FInstrument.cpp - Recording function entry and exit ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file adds print statements to function entry and exit for interesting
// functions
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hello"
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
using namespace llvm;

STATISTIC(FCounter, "Counts number of functions profiled");

namespace {
  // Hello2 - The second implementation with getAnalysisUsage implemented.
  struct FInstrument : public ModulePass {
    static char ID; // Pass identification, replacement for typeid

    LLVMContext *Context;
    Constant *PrintFunc;

    FInstrument() : ModulePass(ID) {}
    bool runOnModule(Module &M) override {
      

      Context = (&M.getContext());
      const Type *SBP = Type::getInt8PtrTy(*Context);

      PrintFunc = M.getOrInsertFunction("printf",Type::getInt32Ty(*Context), SBP, true, (Type*)0);
      
      for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
        if (F->isDeclaration()) continue;

	std::vector<BasicBlock*> exitBlocks;
       
	FInstrument::instrumentEntry(F);

	for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
	  if(isa<ReturnInst>(BB->getTerminator())){
	    exitBlocks.push_back(BB);
	  }
	}
	FInstrument::instrumentExits(F, exitBlocks);
      }
      return true;
    }
    
    void instrumentEntry(Function *F){
      FCounter++;
      BasicBlock &Entry = F->getEntryBlock();
      Instruction *First = Entry.begin();
      IRBuilder<> IRB(First);

      std::string message = "Entring " + F->getName().str() + " \n";
      Value *MessageString = IRB.CreateGlobalString(message);
      Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
      IRB.CreateCall(PrintFunc, MessagePtr); 
    }

    void instrumentExits(Function *F, std::vector<BasicBlock*> exitBlocks){
      for (unsigned i=0; i != exitBlocks.size(); ++i){
	//ReturnInst *Ret = cast<ReturnInst>(exitBlocks[i]->getTerminator());
	IRBuilder<> IRB(exitBlocks[i]->getTerminator());
	std::string message = "Exiting " + F->getName().str() + " \n";
	Value *MessageString = IRB.CreateGlobalString(message);
	Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
	IRB.CreateCall(PrintFunc, MessagePtr);
      }
    }

    // We don't modify the program, so we preserve all analyses.
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      //AU.setPreservesAll();
    }
  };
}

char FInstrument::ID = 0;

static void registerMyPass(const PassManagerBuilder &,
                           PassManagerBase &PM) {
  PM.add(new FInstrument());
}
static RegisterStandardPasses
RegisterMyPass(PassManagerBuilder::EP_EnabledOnOptLevel0,
	       registerMyPass);


static RegisterPass<FInstrument>
Y("finstr", "add print statements to function entry and exit");
