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

  // std::set<std::string> wList = {
  //   "_ZN12nsThreadPool14ShutdownThreadEP9nsIThread",
  //   "_ZN12nsThreadPool8ShutdownEv",
  //   "_ZN15nsThreadManager8ShutdownEv",
  //   "_ZN21nsThreadShutdownEvent3RunEv",
  //   "_ZN21nsThreadShutdownEventC2EP8nsThreadP23nsThreadShutdownContext",
  //   "_ZN21nsThreadShutdownEventD0Ev",
  //   "_ZN21nsThreadShutdownEventD2Ev",
  //   "_ZN24nsThreadShutdownAckEvent3RunEv",
  //   "_ZN24nsThreadShutdownAckEventC2EP23nsThreadShutdownContext",
  //   "_ZN24nsThreadShutdownAckEventD0Ev",
  //   "_ZN24nsThreadShutdownAckEventD2Ev",
  //   "_ZN8nsThread16ShutdownRequiredEv",
  //   "_ZN8nsThread8ShutdownEv",
  //   "_ZN8nsThread16ProcessNextEventEbPb",
  //   "_ZN8nsThread16DispatchInternalEP11nsIRunnablejPNS_19nsNestedEventTargetE",
  //   "_ZN8nsThread19nsNestedEventTarget8DispatchEP11nsIRunnablej",
  //   "_Z23NS_ProcessPendingEventsP9nsIThreadj",
  //   "_ZN20nsThreadSyncDispatch3RunEv"
  // };

  // void FInstrument::readBlacklist(){
  //   std::string line;
  //   std::ifstream skippedFunctionsFile ("/home/anirudh/blacklist20.txt");
  //   std::ifstream skippedDirsFile ("/home/anirudh/blacklistdirs.txt");
  //   if (skippedFunctionsFile.is_open()){
  // 	while ( getline (skippedFunctionsFile,line) ){
  // 	    skipped.insert(line);
  // 	}
  // 	skippedFunctionsFile.close();
  //   }
  //   if (skippedDirsFile.is_open()){
  // 	while ( getline (skippedDirsFile,line) ){
  // 	    skippedDirs.insert(line);
  // 	}
  // 	skippedDirsFile.close();
  //   }

  //   //llvm::outs() << "Populated skipped " << skipped.size() << "\n";
  // }

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
      
      // if(skipped.find(FName) != skipped.end() && (wList.find(FName) == wList.end())){
      // 	//llvm::outs() << "Skipping already skipped function " << FName << "\n";
      // 	continue;
      // }


      // // Don't instrument functions to do with nsCOMPtr and nsRefPtr and nsCOMArray
      // // and cycle collection
      // if(startsWith(FName, "_ZN10nsCOMArray") ||
      // 	 startsWith(FName, "_ZNK10nsCOMArray") ||
      // 	 startsWith(FName, "_ZN13nsTArray") ||
      // 	 startsWith(FName, "_ZNK13nsTArray") ||
      // 	 startsWith(FName, "_ZNK8nsCOMPtr") ||
      // 	 startsWith(FName, "_ZN8nsCOMPtr") ||
      // 	 startsWith(FName, "_ZNK9nsAutoPtr") ||
      // 	 startsWith(FName, "_ZN9nsAutoPtr") ||
      // 	 startsWith(FName, "_ZNK8nsRefPtr") ||
      // 	 startsWith(FName, "_ZN8nsRefPtr") ||
      // 	 startsWith(FName, "_ZN15nsGetterAddRefs") ||
      // 	 startsWith(FName, "NS_IsCycleCollectorThread_P") ||
      // 	 startsWith(FName, "NS_AtomicDecrementRefcnt") ||
      // 	 startsWith(FName, " NS_AtomicIncrementRefcnt") ||
      // 	 startsWith(FName, "_ZN16already_AddRefed") ||
      // 	 startsWith(FName, "xpc_UnmarkGrayObject") ||
      // 	 startsWith(FName, "xpc_UnmarkNonNullGrayObject") ||
      // 	 startsWith(FName, "_ZN10nsDocument15cycleCollection")
      // 	 ){
      // 	skipped.insert(FName);
      // 	//llvm::outs() << "Skipping smart pointer function " << FName << "\n";
      // 	continue;
      // }
     
      // // discard js and std functions, QueryInterface, AddRef, Release, cycle collection stuff
      // std::string demangled = demangleFunctionName(FName);
      // if(startsWith(demangled, "js::") || 
      // 	 startsWith(demangled, "JS::") || 
      // 	 FName.find("CCParticipant") != std::string::npos ||
      // 	 FName.find("cycleCollection") != std::string::npos ||
      // 	 FName.find("ArrayLength") != std::string::npos ||
      // 	 FName.find("nsDefaultComparator") != std::string::npos ||
      // 	 FName.find("autoJArray") != std::string::npos ||
      // 	 FName.find("GetStyleDisplay") != std::string::npos ||
      // 	 FName.find("XPCOM_MIN") != std::string::npos ||
      // 	 endsWith(FName, "QueryInterfaceERK4nsIDPPv") || 
      // 	 endsWith(FName, "AddRefEv") ||
      // 	 endsWith(FName, "ReleaseEv") ||
      // 	 startsWith(demangled, "std::")){
      // 	skipped.insert(FName);
      // 	//llvm::outs() << "Skipping JS/std::/QI " << FName << "\n";
      // 	continue;
      // }
      
      // // discard some frequent data structure access functions/other functions
      // if(startsWith(demangled, "nsAttrName::") ||
      // 	 startsWith(demangled, "nsAttrValue::") ||
      // 	 startsWith(demangled, "nsAttrAndChildArray::") ||
      // 	 startsWith(demangled, "nsIContent::GetID") ||
      // 	 startsWith(demangled, "nsJSContext::MaybePokeCC") ||
      // 	 startsWith(demangled, "nsAutoTObserverArray") ||
      // 	 startsWith(demangled, "nsAutoArrayPtr") ||
      // 	 startsWith(demangled, "mozilla::MillisecondsToMediaTime") ||
      // 	 startsWith(demangled, "mozilla::LinkedListElement") ||
      // 	 startsWith(demangled, "mozilla::BloomFilter") ||
      // 	 startsWith(demangled, "nsEventStates::") ||
      // 	 startsWith(demangled, "nsWrapperCache::") ||
      // 	 startsWith(demangled, "nsINode::") ||
      // 	 startsWith(demangled, "nsStyleContext::") ||
      // 	 startsWith(demangled, "mozilla::safebrowsing::") ||
      // 	 startsWith(demangled, "int mozilla::safebrowsing::") ||
      // 	 startsWith(demangled, "vp8_") ||
      // 	 startsWith(demangled, "decode_") ||
      // 	 startsWith(demangled, "nsReadingIterator") ||
      // 	 startsWith(demangled, "nsCharTraits") ||
      // 	 startsWith(demangled, "mozilla::DebugOnly<bool>") ||
      // 	 startsWith(demangled, "bool operator!=<unsigned short>") ||
      // 	 startsWith(demangled, "unsigned int mozilla::AddToHash") ||
      // 	 startsWith(demangled, "unsigned int const& NS_MIN") ||
      // 	 startsWith(demangled, "nsCRT::IsAsciiSpace") ||
      // 	 startsWith(demangled, "nsCycleCollectingAutoRefCnt")
      // 	 ){
      // 	skipped.insert(FName); 
      // 	//llvm::outs() << "Skipping JS or std " << FName << "\n";
      // 	continue;
      // }
      
      // std::map<std::string,std::string>::const_iterator search = funcNameToDirName.find(FName);
      // std::string dirName;
      // if(search != funcNameToDirName.end()) {
      //   dirName = search->second;
      // }

      // bool found = false;
      // for (std::set<std::string>::iterator it = skippedDirs.begin(); it != skippedDirs.end(); ++it){
      // 	if(dirName.find(*it) != std::string::npos){
      // 	  found = true;
      // 	  break;
      // 	}
      // }
      
      // we avoid skipping over functions in blacklisted directories that
      // we must instrument - the fn must not be in our whitelist
      // if(found && (wList.find(FName) == wList.end())){
      // 	//llvm::outs() << "Early Skipping " << FName << "\n";
      // 	skipped.insert(FName);
      // 	continue;
      // }

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
      
      // if(demangled.find("nsXBLBinding::InstallImplementation") != std::string::npos){
      // 	llvm::outs() << "Before: \n";
      // 	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
      // 	  llvm::outs() << *I << "\n";
      // }

      FInstrument::instrumentEntry(F);

      // if(demangled.find("nsXBLBinding::InstallImplementation") != std::string::npos){
      // 	llvm::outs() << "After Entry: \n";
      // 	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
      // 	  llvm::outs() << *I << "\n";
      // }

      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
      	if(isa<ReturnInst>(BB->getTerminator())){
      	  exitBlocks.push_back(BB);
      	}
      }

      FInstrument::instrumentExits(F, exitBlocks);
      
      // if(demangled.find("nsXBLBinding::InstallImplementation") != std::string::npos){
      // 	llvm::outs() << "After entry exit: \n";
      // 	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
      // 	  llvm::outs() << *I << "\n";
      // }

    }
    
    // std::vector<std::string> v_intersection;
 
    // std::set_intersection(skipped.begin(), skipped.end(),
    //                       instrumented.begin(), instrumented.end(),
    //                       std::back_inserter(v_intersection));
    // if(!v_intersection.empty())
    //   llvm::outs() << "Suspicious ";
    // for (std::vector<std::string>::iterator it = v_intersection.begin(); it != v_intersection.end(); ++it)
    //   llvm::outs() << *it << ' ';
    // for(std::set<std::string>::const_iterator i = skipped.begin(); i != skipped.end(); ++i) {
    //   // process i
    //   llvm::outs() << "Skipping:" << *i << "\n";
    // }
    return false;
  }
    
  void FInstrument::instrumentEntry(Function *F){
    FCounter++;
    //errs() << *F << "\n";
    //IRBuilder<> IRB(F->getEntryBlock().getFirstNonPHI());
    IRBuilder<> IRB(F->getEntryBlock().getFirstInsertionPt());
    //BasicBlock &Entry = F->getEntryBlock();

    // The following ran into trouble with functions that began with if/for
    // Instruction *First = Entry.begin();
    // IRBuilder<> IRB(First);

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
    Value *MessageString = IRB.CreateGlobalString(demangleFunctionName(F->getName().str()));
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
      Value *MessageString = IRB.CreateGlobalString(demangleFunctionName(name));
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
