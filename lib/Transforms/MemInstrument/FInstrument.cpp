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

#include "FInstrument.h"

using namespace llvm;

namespace MemInstrument {
  // Hello2 - The second implementation with getAnalysisUsage implemented.
  bool FInstrument::runOnModule(Module &M) {
    

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
    
  void FInstrument::instrumentEntry(Function *F){
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
    //flag = true;
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

  void FInstrument::instrumentExits(Function *F, std::vector<BasicBlock*> exitBlocks){
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
      // flag = true;	
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
  void FInstrument::getAnalysisUsage(AnalysisUsage &AU) const {
    //AU.setPreservesAll();
  }

}


char MemInstrument::FInstrument::ID = 0;

// static void registerMyPass(const PassManagerBuilder &,
//                            PassManagerBase &PM) {
//   PM.add(new FInstrument());
// }

// static RegisterStandardPasses
// RegisterMyPass(PassManagerBuilder::EP_EnabledOnOptLevel0,
// 	       registerMyPass);


// static RegisterPass<FInstrument>
// Y("finstr", "add print statements to function entry and exit");
// }
