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

#include <cxxabi.h>

#include <set>

using namespace llvm;

STATISTIC(FCounter, "Counts number of functions profiled");

namespace {
  // Hello2 - The second implementation with getAnalysisUsage implemented.
  struct FInstrument : public ModulePass {
    static char ID; // Pass identification, replacement for typeid

    LLVMContext *Context;
    Constant *PrintFunc;
    Constant *PrintF;
    
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

    FInstrument() : ModulePass(ID) {}
    bool runOnModule(Module &M) override {
      

      Context = (&M.getContext());
      // The code that follows causes clang to crash, for whatever reason
      // const Type *SBP = PointerType::getUnqual(Type::getInt8Ty(*Context));

      // PrintFunc = M.getOrInsertFunction("printf", Type::getInt32Ty(*Context), SBP, true, (Type*)0);
      // Function *Printf = dyn_cast<Function>(PrintFunc);
      // Printf->setCallingConv(CallingConv::C);
      // errs() << "Runs the thing! \n";
      // errs() << Printf->getName() << "\n";
      
      std::vector<Type*> Params;
      Params.push_back(PointerType::getUnqual(Type::getInt8Ty(*Context)));
      // Get the printf() function (takes an i8* followed by variadic parameters)
      PrintF = M.getOrInsertFunction("printf",
      					       FunctionType::get(Type::getVoidTy(*Context), Params, true));
      //errs() << PrintF->getName() << "\n";

      for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
        if (F->isDeclaration()) continue;
	if (F->getName().str() == "free" || F->getName().str() == "printf") continue;
	std::vector<BasicBlock*> exitBlocks;
       
	FInstrument::instrumentEntry(F);

	for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
	  if(isa<ReturnInst>(BB->getTerminator())){
	    exitBlocks.push_back(BB);
	  }
	}
	FInstrument::instrumentExits(F, exitBlocks);
      }
      return false;
    }
    
    void instrumentEntry(Function *F){
      FCounter++;
      //errs() << *F << "\n";
      BasicBlock &Entry = F->getEntryBlock();
      Instruction *First = Entry.begin();
      IRBuilder<> IRB(First);
      std::string name("Crap \n");
      //errs() << F->getName() << "\n";
      //errs() << F->getName().str() << "\n";
      name = F->getName().str();
      std::string unmangled(demangleFunctionName(name));
      bool flag = shouldInstrument(unmangled, whiteList);
      // for (std::set<std::string>::iterator it=whiteList.begin(); it!=whiteList.end(); ++it){
      // 	if (it->find(unmangled) != std::string::npos && unmangled.length() > 10) {
      // 	  errs () << "unmangled " << unmangled;
      // 	  flag = true;
      // 	  break;
      // 	}
      // }
      if(false == flag)
      	return;

      std::string message("Entering "+unmangled+" \n");
      // errs() << "Suspect \n";
      Value *MessageString = IRB.CreateGlobalString(message);
      Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
      IRB.CreateCall(PrintF, MessagePtr); 
    }

    void instrumentExits(Function *F, std::vector<BasicBlock*> exitBlocks){
      for (unsigned i=0; i != exitBlocks.size(); ++i){
	//ReturnInst *Ret = cast<ReturnInst>(exitBlocks[i]->getTerminator());
	IRBuilder<> IRB(exitBlocks[i]->getTerminator());
	std::string name("Crap \n");
	// errs() << F->getName() << "\n";
	// errs() << F->getName().str() << "\n";
	//name = demangleFunctionName(F->getName().str());
	// if(whiteList.find(name) == whiteList.end()){
	//   continue;
	// }
	
	name = F->getName().str();
	std::string unmangled(demangleFunctionName(name));

 	bool flag = shouldInstrument(unmangled, whiteList);
	
	// for (std::set<std::string>::iterator it=whiteList.begin(); it!=whiteList.end(); ++it){
	//   if (it->find(unmangled) != std::string::npos && unmangled.length() > 10) {
	//     flag = true;
	//     break;
	//   }
	// }
	if(false == flag)
	  return;
	
	std::string message("Exiting "+unmangled+" \n");
	Value *MessageString = IRB.CreateGlobalString(message);
	Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
	IRB.CreateCall(PrintF, MessagePtr);
      }
    }

    // We don't modify the program, so we preserve all analyses.
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      //AU.setPreservesAll();
    }

    bool shouldInstrument(std::string name, std::set<std::string> whiteList){
      if(whiteList.find(name) != whiteList.end())
	return true;
      return false;
    }
    
    std::string demangleFunctionName(std::string func) {
      std::string ret(func);

      // Check for name mangling. C++ functions will always start with _Z
      // Demangled form is processed to remove type information.

      if(func.length() >= 2 && func[0] == '_' && func[1] == 'Z') {
	int stat;

	char *test = abi::__cxa_demangle(func.c_str(), NULL, NULL, &stat);
	if(NULL == test)
	  return "Demangling Failed on " + func;


	std::string demangled = test;
	free(test);

	// Select up to the first ( to only insert function name
	size_t endpos = demangled.find("(");
      
	// Templated functions will have type information first, so skip to the
	// first space.
	size_t startpos = demangled.find(" ");
	if(startpos < endpos) {
	  // skip until after the space
	  ++startpos;
	  // also modify endpos to the first '<' to remove template info
	  endpos = demangled.find("<") - startpos;
	} else {
	  // regular C++ function, no template info to remove
	  startpos = 0;
	}
	
	ret = demangled.substr(startpos,endpos);
      }

      return ret;
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
