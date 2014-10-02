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

    std::vector<Type*> P;
    P.push_back(IntptrTy);
    P.push_back(PointerType::getUnqual(Type::getInt8Ty(*Context)));
    DispatchTracker = M.getOrInsertFunction("dispatchInstrument",
					    FunctionType::get(Type::getVoidTy(*Context), P, false));

    
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      std::string FName = F->getName().str();
      std::string demangledName = demangleFunctionName(FName);
      //llvm::outs() << "Processing " << FName << " --> " << demangledName << "\n";
      if(demangledName.find("::Run()") != std::string::npos ||
	 demangledName.find("nsRunnableMethodImpl") != std::string::npos){
	//llvm::outs() << "Instrumenting Runnable " << demangledName << "\n";
	RunnableInstrument::instrumentEntryExits(F, demangledName);
      }

    // for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
    //   if (F->isDeclaration()) continue;
    //   std::string FName = F->getName().str();
    //   std::string demangledName = demangleFunctionName(FName);
      
      // Don't instrument uninteresting callers
      if(FName.find("NS_DispatchToCurrentThread") != std::string::npos ||
	 FName.find("NS_DispatchToMainThread") != std::string::npos){
	// llvm::outs() << "Skipping " << FName << " --> " << demangledName << "\n";
	continue;
      }
      // if(FName.find("NS_NewThread") != std::string::npos)
      // 	llvm::outs() << "===========Begin Log==============\n";
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
        for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
             BI != BE; ++BI) {
	  // if(FName.find("NS_NewNamedThread") != std::string::npos)
	  //     llvm::outs() << *BI << "\n";
	    
          if (CallInst *CI = dyn_cast<CallInst>(BI)){
	    // if(FName.find("NS_NewNamedThread") != std::string::npos)
	    //   llvm::outs() << "Calling: " << *CI << "\n";
            if (Function * CalledFunc = CI->getCalledFunction()) {
	      std::string name = demangleFunctionName(CalledFunc->getName());
	      //llvm::outs() << "Considering call to " << name << "\n";
              if(name.find("NS_DispatchToCurrentThread") != std::string::npos ||
                 name.find("NS_DispatchToMainThread") != std::string::npos || 
		 name.find("nsThread::Dispatch(") != std::string::npos)
		{
		  // if(FName.find("NS_NewThread") != std::string::npos)
		  //   llvm::outs() << "Instrumenting call to " << name << "in caller " << FName << "\n";  
		RunnableInstrument::InstrumentDispatch(BI, FName);
              }
            }
	    // this is an indirect call
	    else{
	      Type* t = CI->getCalledValue()->getType();
	      FunctionType* ft = cast<FunctionType>(cast<PointerType>(t)->getElementType());
	      std::string sig = getTypeAsString(ft);
	      if(sig.compare("i32 (%class.nsIEventTarget*, %class.nsIRunnable*, i32)") == 0){
		llvm::outs() << "Instrumenting dispatch at " << getSourceInfoAsString(CI, "") << "\n";
		RunnableInstrument::InstrumentDispatch(BI, FName);
	      }
	    }
          }
        }
      }
      // if(FName.find("NS_NewThread") != std::string::npos)
      // 	llvm::outs() << "===========End Log==============\n";

    }
	  

    //   std::vector<BasicBlock*> exitBlocks;
       
    //   RunnableInstrument::instrumentEntry(F, demangledName);

    //   for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
    // 	if(isa<ReturnInst>(BB->getTerminator())){
    // 	  exitBlocks.push_back(BB);
    // 	}
    //   }
    //   RunnableInstrument::instrumentExits(F, exitBlocks, demangledName);
    // }
    return false;
  }
  

  void RunnableInstrument::InstrumentDispatch(BasicBlock::iterator &BI,
					    std::string demangledName){
  llvm::Instruction &IN = *BI;
  IRBuilder<> IRB(BI);
  Value* Addr = IN.getOperand(0)->stripPointerCasts();
  Value *AddrLong = IRB.CreatePointerCast(Addr, IntptrTy);
  Value *MessageString = IRB.CreateGlobalString(demangledName);
  Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
  std::vector<llvm::Value*> paramArrayRef;
  paramArrayRef.push_back(AddrLong);
  paramArrayRef.push_back(MessagePtr);
  IRB.CreateCall(DispatchTracker, paramArrayRef);
  }

  void RunnableInstrument::instrumentEntryExits(Function *F,
					      std::string demangledName){
    std::vector<BasicBlock*> exitBlocks;
    RunnableInstrument::instrumentEntry(F, demangledName);

    for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
      if(isa<ReturnInst>(BB->getTerminator())){
	exitBlocks.push_back(BB);
      }
    }
    RunnableInstrument::instrumentExits(F, exitBlocks, demangledName);
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
