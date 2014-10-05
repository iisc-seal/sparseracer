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
#include <vector>
#include <algorithm>
#include <fstream>

using namespace llvm;

namespace MemInstrument {

  void FInstrument::readBlacklist(){
    std::string line;
    std::ifstream skippedFunctionsFile ("/home/anirudh/blacklist.txt");
    std::ifstream skippedDirsFile ("/home/anirudh/blacklistdirs.txt");
    if (skippedFunctionsFile.is_open()){
	while ( getline (skippedFunctionsFile,line) ){
	    skipped.insert(line);
	}
	skippedFunctionsFile.close();
    }
    if (skippedDirsFile.is_open()){
	while ( getline (skippedDirsFile,line) ){
	    skippedDirs.insert(line);
	}
	skippedDirsFile.close();
    }

    llvm::outs() << "Populated skipped " << skipped.size() << "\n";
  }
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
    Params.push_back(PointerType::getUnqual(Type::getInt8Ty(*Context))); // fName as the first
    Params.push_back(Type::getInt32Ty(*Context));  // enter/exit bit as the second
    // Get the logger function (takes an i8* followed by an int)
    Logger = M.getOrInsertFunction("fInstrument",
				   FunctionType::get(Type::getVoidTy(*Context), Params, false));
    //errs() << PrintF->getName() << "\n";

    std::map<std::string, std::string> funcNameToDirName = getDebugInformation(M);

    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      std::string FName = F->getName().str();
      if (F->isDeclaration()) continue;
      //llvm::outs() << FName << "\n";

      // Don't instrument functions to do with nsCOMPtr and nsRefPtr and nsCOMArray
      // and cycle collection
      if(startsWith(FName, "_ZN10nsCOMArray") ||
	 startsWith(FName, "_ZNK10nsCOMArray") ||
	 startsWith(FName, "_ZN13nsTArray") ||
	 startsWith(FName, "_ZNK13nsTArray") ||
	 startsWith(FName, "_ZNK8nsCOMPtr") ||
	 startsWith(FName, "_ZN8nsCOMPtr") ||
	 startsWith(FName, "_ZNK9nsAutoPtr") ||
	 startsWith(FName, "_ZN9nsAutoPtr") ||
	 startsWith(FName, "_ZNK8nsRefPtr") ||
	 startsWith(FName, "_ZN8nsRefPtr") ||
	 startsWith(FName, "_ZN15nsGetterAddRefs") ||
	 startsWith(FName, "_ZN16already_AddRefed") ||
	 startsWith(FName, "_ZN10nsDocument15cycleCollection")
	 ){
	llvm::outs() << "Skipping smart pointer function " << FName << "\n";
	continue;
      }

      if(skipped.find(FName) != skipped.end()){
	llvm::outs() << "Skipping already skipped function " << FName << "\n";
	continue;
      }
      
      std::string demangled = demangleFunctionName(FName);
      if(demangleFunctionName(FName).find("js::") != std::string::npos){
	llvm::outs() << "Skipping JS " << FName << "\n";
	continue;
      }

      
      std::map<std::string,std::string>::const_iterator search = funcNameToDirName.find(FName);
      std::string dirName;
      if(search != funcNameToDirName.end()) {
        dirName = search->second;
      }

      bool found = false;
      for (std::set<std::string>::iterator it = skippedDirs.begin(); it != skippedDirs.end(); ++it){
	if(dirName.find(*it) != std::string::npos){
	  found = true;
	  break;
	}
      }
      if(found){
	llvm::outs() << "Early Skipping " << FName << "\n";
	skipped.insert(FName);
	continue;
      }

      // if(dirName.find("nsprpub") != std::string::npos ||
      // 	 dirName.find("xpcom") != std::string::npos ||
      // 	 dirName.find("mfbt") != std::string::npos ||
      // 	 dirName.find("mozglue") != std::string::npos ||
      // 	 dirName.find("js/") != std::string::npos ||
      // 	 dirName.find("db/") != std::string::npos ||
      // 	 dirName.find("memory/") != std::string::npos ||
      // 	 dirName.find("ipc") != std::string::npos ||
      // 	 dirName.find("gfx/") != std::string::npos ||
      // 	 dirName.find("layout/style") != std::string::npos ||
      // 	 dirName.find("security") != std::string::npos 
      // 	 ){
      // 	llvm::outs() << "Early Skipping " << FName << "\n";
      // 	skipped.insert(FName);
      // 	continue;
      // }

      if(!shouldInstrumentFunction(F, FName, skippedDirs))
	continue;

      //if (F->getName().str() == "free" || F->getName().str() == "printf") continue;
      std::vector<BasicBlock*> exitBlocks;

      instrumented.insert(FName);
      FInstrument::instrumentEntry(F);

      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
	if(isa<ReturnInst>(BB->getTerminator())){
	  exitBlocks.push_back(BB);
	}
      }
      FInstrument::instrumentExits(F, exitBlocks);
    }
    
    std::vector<std::string> v_intersection;
 
    std::set_intersection(skipped.begin(), skipped.end(),
                          instrumented.begin(), instrumented.end(),
                          std::back_inserter(v_intersection));
    if(!v_intersection.empty())
      llvm::outs() << "Suspicious ";
    for (std::vector<std::string>::iterator it = v_intersection.begin(); it != v_intersection.end(); ++it)
      llvm::outs() << *it << ' ';
    
    return false;
  }
    
  void FInstrument::instrumentEntry(Function *F){
    FCounter++;
    //errs() << *F << "\n";
    BasicBlock &Entry = F->getEntryBlock();
    Instruction *First = Entry.begin();
    IRBuilder<> IRB(First);
    // std::string name("Crap \n");
    //errs() << F->getName() << "\n";
    //errs() << F->getName().str() << "\n";
    // name = F->getName().str();
    // std::string unmangled(demangleFunctionName(name));
    // bool flag = shouldInstrumentFunction(unmangled, whiteList);
    //flag = true;
    // for (std::set<std::string>::iterator it=whiteList.begin(); it!=whiteList.end(); ++it){
    // 	if (it->find(unmangled) != std::string::npos && unmangled.length() > 10) {
    // 	  errs () << "unmangled " << unmangled;
    // 	  flag = true;
    // 	  break;
    // 	}
    // }
    // if(false == flag)
    //   return;

    
    //Value *ThreadId =  ConstantInt::get(Type::getInt64Ty(*Context), pthread_self());
    Value *MessageString = IRB.CreateGlobalString(F->getName().str());
    Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
    Value *MessageType =  ConstantInt::get(Type::getInt32Ty(*Context), 1);

    std::vector<llvm::Value*> paramArrayRef;
    paramArrayRef.push_back(MessagePtr);
    paramArrayRef.push_back(MessageType);
    // paramArrayRef.push_back(ThreadId);

    IRB.CreateCall(Logger, paramArrayRef); 
  }

  void FInstrument::instrumentExits(Function *F, std::vector<BasicBlock*> exitBlocks){
    for (unsigned i=0; i != exitBlocks.size(); ++i){
      //ReturnInst *Ret = cast<ReturnInst>(exitBlocks[i]->getTerminator());
      IRBuilder<> IRB(exitBlocks[i]->getTerminator());
      //std::string name("Crap \n");
      // errs() << F->getName() << "\n";
      // errs() << F->getName().str() << "\n";
      //name = demangleFunctionName(F->getName().str());
      // if(whiteList.find(name) == whiteList.end()){
      //   continue;
      // }
	
      std::string name = F->getName().str();
      // std::string unmangled(demangleFunctionName(name));
      
      // bool flag = shouldInstrumentFunction(unmangled, whiteList);
      // flag = true;	
      // for (std::set<std::string>::iterator it=whiteList.begin(); it!=whiteList.end(); ++it){
      //   if (it->find(unmangled) != std::string::npos && unmangled.length() > 10) {
      //     flag = true;
      //     break;
      //   }
      // }
      // if(false == flag)
      // return;

      //std::string message("exiting(%lu, "+ name +")");
      //Value *ThreadId =  ConstantInt::get(Type::getInt64Ty(*Context), pthread_self());
      Value *MessageString = IRB.CreateGlobalString(name);
      Value *MessagePtr = IRB.CreateBitCast(MessageString, IRB.getInt8PtrTy());
      Value *MessageType =  ConstantInt::get(Type::getInt32Ty(*Context), 0);

      std::vector<llvm::Value*> paramArrayRef;

      paramArrayRef.push_back(MessagePtr);
      paramArrayRef.push_back(MessageType);
      //paramArrayRef.push_back(ThreadId);
      
      IRB.CreateCall(Logger, paramArrayRef); 

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
