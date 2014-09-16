//===- RunnableInstrument.cpp - Recording entry and exit into Run() methods ===//
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

#include "RunnableInstrument.h"

using namespace llvm;

namespace MemInstrument {

  bool RunnableInstrument::runOnModule(Module &M) {
    Context = (&M.getContext());
      
    std::vector<Type*> Params;
    IntptrTy = Type::getInt64Ty(*Context);
    Params.push_back(IntptrTy);
    Params.push_back(PointerType::getUnqual(Type::getInt8Ty(*Context))); // fName as the first
    Params.push_back(Type::getInt32Ty(*Context));  // enter/exit bit as the second
    // Get the logger function (takes an i8* followed by an int)
    Logger = M.getOrInsertFunction("rInstrument",
				   FunctionType::get(Type::getVoidTy(*Context), Params, false));


    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      std::string FName = F->getName().str();
      std::string demangledName = demangleFunctionName(FName);

      if(!(demangledName.find("::Run()") != std::string::npos &&
	   demangledName.find("nsRunnableMethodImpl") == std::string::npos))
	continue;

      std::vector<BasicBlock*> exitBlocks;
       
      RunnableInstrument::instrumentEntry(F, demangledName);

      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
	if(isa<ReturnInst>(BB->getTerminator())){
	  exitBlocks.push_back(BB);
	}
      }
      RunnableInstrument::instrumentExits(F, exitBlocks, demangledName);
    }
    return false;
  }
    
  void RunnableInstrument::instrumentEntry(Function *F, std::string demangledName){
    BasicBlock &Entry = F->getEntryBlock();
    Instruction *First = Entry.begin();
    IRBuilder<> IRB(First);
    
    assert(!F->arg_empty());
    Value *thisVal =  F->arg_begin();
    AllocaInst *Addr = IRB.CreateAlloca(thisVal->getType(), 0, "this.addrFake");
    IRB.CreateStore(thisVal, Addr);
    Value *this1 = IRB.CreateLoad(Addr, "t1Fake");
    Value *thisPtr = IRB.CreatePointerCast(this1, IntptrTy);

    //Value *ThreadId =  ConstantInt::get(Type::getInt64Ty(*Context), pthread_self());
    Value *MessageString = IRB.CreateGlobalString(demangledName);
    Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
    Value *MessageType =  ConstantInt::get(Type::getInt32Ty(*Context), 1);

    std::vector<llvm::Value*> paramArrayRef;
    paramArrayRef.push_back(thisPtr);
    paramArrayRef.push_back(MessagePtr);
    paramArrayRef.push_back(MessageType);
    // paramArrayRef.push_back(ThreadId);

    IRB.CreateCall(Logger, paramArrayRef); 
  }

  void RunnableInstrument::instrumentExits(Function *F, std::vector<BasicBlock*> exitBlocks, 
				    std::string demangledName){
    for (unsigned i=0; i != exitBlocks.size(); ++i){
      IRBuilder<> IRB(exitBlocks[i]->getTerminator());
      assert(!F->arg_empty());
      Value *thisVal =  F->arg_begin();
      AllocaInst *Addr = IRB.CreateAlloca(thisVal->getType(), 0, "this.addrFake");
      IRB.CreateStore(thisVal, Addr);
      Value *this1 = IRB.CreateLoad(Addr, "t1Fake");
      Value *thisPtr = IRB.CreatePointerCast(this1, IntptrTy);

      Value *MessageString = IRB.CreateGlobalString(demangledName);
      Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
      Value *MessageType =  ConstantInt::get(Type::getInt32Ty(*Context), 0);

      std::vector<llvm::Value*> paramArrayRef;
      paramArrayRef.push_back(thisPtr);
      paramArrayRef.push_back(MessagePtr);
      paramArrayRef.push_back(MessageType);

      IRB.CreateCall(Logger, paramArrayRef); 

    }
  }

    // We don't modify the program, so we preserve all analyses.
  void RunnableInstrument::getAnalysisUsage(AnalysisUsage &AU) const {
    //AU.setPreservesAll();
  }

}


char MemInstrument::RunnableInstrument::ID = 0;

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
