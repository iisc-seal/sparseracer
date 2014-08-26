//=- AllocFreeInstrument.cpp - Print addresses malloc'd/freed, new'd/deleted" -//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file adds instructions to IR to instrument memory allocation functions.
// Currently, new, new[], delete, delete[], malloc and free are supported.
//
//===----------------------------------------------------------------------===//

#include "AllocFreeInstrument.h"

using namespace llvm;

namespace MemInstrument {

  bool AllocFreeInstrument::runOnModule(Module &M) {
    
    DataLayoutPass *DLP = getAnalysisIfAvailable<DataLayoutPass>();
    if (!DLP)
      return false;
    DL = &DLP->getDataLayout();
    
    Context = &(M.getContext());
    int LongSize = DL->getPointerSizeInBits();
    IntptrTy = Type::getIntNTy(*Context, LongSize);

    Type *Void = Type::getVoidTy(*Context);
    const Type *SBP = Type::getInt8PtrTy(*Context);
    std::string allocInstrFnName("_Z8mopAllociiPcS_S_");
    std::string dallocInstrFnName("_Z10mopDeallociiPcS_S_");
    MAllocFn = M.getOrInsertFunction("mopAlloc", Void,
				     IntptrTy, Type::getInt64Ty(*Context), 
				     SBP, SBP, SBP,
				     (Type*)0);
    MDallocFn = M.getOrInsertFunction("mopDealloc", Void,
				      IntptrTy, Type::getInt64Ty(*Context), 
				      SBP, SBP, SBP,
				      (Type*)0);
    (cast<Function>(MAllocFn))->setCallingConv(CallingConv::C);
    (cast<Function>(MDallocFn))->setCallingConv(CallingConv::C);
    
    // Figure out TargetLibraryInfo.
    Triple TargetTriple(M.getTargetTriple());
    const TargetLibraryInfo *TLI = new TargetLibraryInfo(TargetTriple); 
    
    std::map<std::string, std::string> funcNameToDirName = getDebugInformation(M);
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      std::string fName = F->getName().str();
      std::string dirName = "";
      // Try to abort early based on the directories to be instrumented 
      std::map<std::string,std::string>::const_iterator search = funcNameToDirName.find(fName);
      if(search != funcNameToDirName.end()) {
        dirName = search->second;
      }
      // also, abort if function is related to the allocation of threads
      // we don't want to read thread id's before the memory is allocated
      // if(dirName.compare("")!=0)
      //   if(!shouldInstrumentDirectory(dirName) 
      
      // if(dirName.find("nsprpub/pr/src") != std::string::npos
      // 	   || dirName.find("ipc/chromium/src/base") !=std::string::npos)
      //     continue;
      // don't instrument within alloc and free functions
      // becuse this will cause duplicates in the trace
      const bool found = (allocFunctions.find(fName) != allocFunctions.end() ||
			  freeFunctions.find(fName) != freeFunctions.end());
      if(found)
	continue;
      // don't instrument syslog
      if(fName.find("syslog")!=std::string::npos)
	continue;
      if(// fName.find("PR_GetThreadID")!=std::string::npos || 
	 // fName.find("PR_GetCurrentThread")!=std::string::npos || 
	 // fName.find("_PR_") != std::string::npos ||
	 // fName.find("pt_") !=std::string::npos || fName.compare("mopAlloc")==0 ||
	 fName.compare("mopDealloc") == 0 || fName.compare("mopInstrument")==0) 
	continue;
      // if (!shouldInstrument(demangleFunctionName(F->getName().str()), whiteList))
      // 	continue;
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
	AllocFreeInstrument::runOnBasicBlock(BB, fName, dirName, TLI);
      }
    }
    return true;
  }
    

  void AllocFreeInstrument::InstrumentDealloc(BasicBlock::iterator &BI, std::string fName, 
					      const TargetLibraryInfo *TLI) {
    
    // Output instruction currently being processed
    llvm::Instruction &IN = *BI;
    //errs() << "Dumping Instruction: ";
    //IN.dump();

    // Get Address and being freed
    Value* Addr = IN.getOperand(0)->stripPointerCasts();
    //errs() << "Dumping Freed Type: ";
    //Addr->stripPointerCasts()->getType()->dump();
    //errs()<<"\n";

    // Get Type Information
    IRBuilder<> IRB(BI);
    Type *OrigPtrTy = Addr->getType();
    //errs() << "Dumping original pointer type: " << "\n";
    //OrigPtrTy->dump();
    //errs() << "\n";
    Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
    //errs() << "Dumping original type: " << "\n";
    //OrigTy->dump();
    //errs() << "\n";

    // Get size of type being freed
    if(!OrigTy->isSized()){
      //errs() << "Failed to track free on :"; 
      //OrigTy->dump();
      ++MissedFrees;
      return;
    }
    assert(OrigTy->isSized());
    //uint32_t TypeSize = DL->getTypeStoreSizeInBits(OrigTy);
    //errs() << "\n"<< "Type Size is: " << TypeSize << "\n";
    uint64_t TypeSize;
    if(!getObjectSize(Addr, TypeSize, DL, TLI, false)){
      TypeSize = DL->getTypeStoreSizeInBits(OrigTy)/8;
    }

    Value *Size =  ConstantInt::get(Type::getInt64Ty(*Context), TypeSize);
    //assert((TypeSize % 8) == 0);
    
    //Cast the address being written to into an int
    Value *AddrLong = IRB.CreatePointerCast(Addr, IntptrTy);
    assert (AddrLong!=NULL);

    //Create a string representing the type being written to
    Value *TypeString = IRB.CreateGlobalString(getTypeAsString(OrigPtrTy));

    //Get a pointer to the string
    Value *TypeStringPtr = IRB.CreateBitCast(TypeString, IRB.getInt8PtrTy());

    //Get a string representing source line number information
    Value *DebugLocationString = IRB.CreateGlobalString(getSourceInfoAsString(BI, fName));

    //Get a pointer to the string
    Value *DebugStringPtr = IRB.CreateBitCast(DebugLocationString, IRB.getInt8PtrTy());

    //Get a string representing the function this write is in
    Value *FunctionNameString = IRB.CreateGlobalString(fName);

    //Get a pointer to the string representing the function name
    Value *FunctionStringPtr = IRB.CreateBitCast(FunctionNameString, IRB.getInt8PtrTy());


    //Track if this is an allocate (or deallocate)
    // Value *Allocate = ConstantInt::get(Type::getInt32Ty(*Context), isAlloc);
    //errs() << "Got here! \n";
    IRB.CreateCall5(MDallocFn, AddrLong, Size, TypeStringPtr, DebugStringPtr, FunctionStringPtr);
  }

  Value* getIntegerValue(Value* val){
    if(isa<llvm::ConstantInt>(val))
      return dyn_cast<llvm::ConstantInt>(val);
    
    return val;
  }

  // TODO : refactor to remove redundant code
  Value* AllocFreeInstrument::getMemSize(CallInst* Original, 
					 const TargetLibraryInfo *TLI, IRBuilder<> IRB){
    Value* MemSize;
    // if(llvm::isOperatorNewLikeFn(Original, TLI) 
    //    || llvm::isMallocLikeFn(Original, TLI))
    if(allocFunctions.find(Original->getCalledFunction()->getName()) != allocFunctions.end()
       && Original->getCalledFunction()->getName().compare("realloc") != 0
       && Original->getCalledFunction()->getName().compare("calloc") != 0)
      {
	//llvm::outs() << "Got here! \n";
      MemSize = getIntegerValue(Original->getOperand(0));
    }
    else if(Original->getCalledFunction()->getName().compare("realloc") == 0)
      MemSize = getIntegerValue(Original->getOperand(1));
    else if(Original->getCalledFunction()->getName().compare("calloc") == 0){
      Value *size1 = getIntegerValue(Original->getOperand(0));
      Value *size2 = getIntegerValue(Original->getOperand(1));
      MemSize = IRB.CreateMul(size1, size2);
    }
    else 
      assert("Trying to instrument unsupported allocator!");
    return MemSize;
  }

  void AllocFreeInstrument::InstrumentAlloc(Instruction* Succ, CallInst *Original, 
					    std::string fName, const TargetLibraryInfo *TLI,
					    IRBuilder<> IRB) {

    // Original->print(llvm::outs());
    // llvm::outs() << "\n";
    // Succ->print(llvm::outs());
    // llvm::outs() << "\n";

    Type *AllocatedType = nullptr;
    if(BitCastInst* BCI = dyn_cast<BitCastInst>(Succ))
      AllocatedType = BCI -> getType();
    else 
      AllocatedType = Original->getType();   

    // if(isMallocLikeFn(Original, TLI)){
    //   PointerType *PType =  llvm::getMallocType(Original, TLI); 
    //   AllocatedType = PType ? PType->getElementType() : 
    // 	Type::getVoidTy(Original->getParent()->getContext());
    // }
   
    // Get the address allocated
    Value *AddrLong = IRB.CreatePointerCast(Original, IntptrTy);
    // errs() << "Address " << *AddrLong << "\n";

    // Get the number of bytes allocated
    Value *MemSize = getMemSize(Original, TLI, IRB);
    // if(isa<llvm::ConstantInt>(Original->getOperand(0)))
    //   MemSize = dyn_cast<llvm::ConstantInt>(Original->getOperand(0));
    // else
    //   MemSize = Original->getOperand(0);
    // errs() << "Size " << *MemSize<< "\n";

    //Create a string representing the type being written to
    Value *TypeString  = IRB.CreateGlobalString(getTypeAsString(AllocatedType));
    //errs() << "Type String" << getTypeAsString(AllocatedType) << "\n";
    //Get a pointer to the string   
    Value *TypeStringPtr = IRB.CreateBitCast(TypeString, IRB.getInt8PtrTy());
    // errs() << "TYS PTr" << *TypeStringPtr<< "\n";
    //Get a string representing source line number information
    Value *DebugLocationString = IRB.CreateGlobalString(getSourceInfoAsString(Original, fName));
    // errs() << "DebugLocPtr" << *DebugLocationString<< "\n";
    //Get a pointer to the string
    Value *DebugStringPtr = IRB.CreateBitCast(DebugLocationString, IRB.getInt8PtrTy());
    // errs() << "DebugStrPtr" << *DebugStringPtr<< "\n";
    //errs() <<"Alloc! \n";
    //Get a string representing the function this write is in
    std::string extra = "";
    if(Original->getCalledFunction()->getName().compare("realloc")==0)
      extra = "realloc";
    Value *FunctionNameString = IRB.CreateGlobalString(extra + " in " + fName);
    
    //Get a pointer to the string representing the function name
    Value *FunctionStringPtr = IRB.CreateBitCast(FunctionNameString, IRB.getInt8PtrTy());


    IRB.CreateCall5(MAllocFn, AddrLong, MemSize, TypeStringPtr, DebugStringPtr, FunctionStringPtr);
    //errs() << "Got Here after call! \n";
    //errs() << *CI << "\n";        
  }

  bool AllocFreeInstrument::runOnBasicBlock(Function::iterator &BB, std::string callerName, 
					    std::string dName, const TargetLibraryInfo *TLI) {

    bool flag = false;
    std::vector< std::pair<Instruction*, Instruction*> > Interesting;
    int count = 0;

    for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
         BI != BE; ++BI) { 
      
      if(flag == true){
	Interesting[count].second = BI;
	count++;
	flag = false;
      }

      if (CallInst * CI = dyn_cast<CallInst>(BI)) {

        // if(dName.compare("")==0){
	//   std::string dirName = getDirName(CI);
	//   if(!shouldInstrumentDirectory(dirName))
	//     continue;
	// }

	if (Function * CalledFunc = CI->getCalledFunction()) {
	  std::string name = CalledFunc->getName();
	  bool found = (freeFunctions.find(name) != freeFunctions.end());
	  if(found){
	    AllocFreeInstrument::InstrumentDealloc(BI, callerName, TLI);
	    continue;
	  }
	  found = (allocFunctions.find(name) != allocFunctions.end());
	  if(found){
	    flag = true;
	    std::pair <Instruction*, Instruction*> allocAndNext = std::make_pair (BI, nullptr);
	    Interesting.push_back(allocAndNext);
	  }
	}
      }
      
    }
    for(std::vector< std::pair<Instruction*, Instruction*> >::size_type i = 0; 
	  i != Interesting.size(); i++) {
      CallInst * CI = dyn_cast<CallInst>(Interesting[i].first);
      Instruction* Succ = Interesting[i].second;
      assert(Succ);
      // if(Succ == CI->getParent()->getTerminator()){
      // 	IRBuilder<> IRB(CI->getParent());
      // 	llvm::outs() << callerName << "\n";
      // 	AllocFreeInstrument::InstrumentAlloc(Succ, CI, callerName, TLI, IRB);
      // }
      
      IRBuilder<> IRB(Succ);
      //llvm::outs() << "Caller Name: " << callerName << "\n";
      AllocFreeInstrument::InstrumentAlloc(Succ, CI, callerName, TLI, IRB);

      // Interesting[i].first->print(llvm::outs());
      // llvm::outs() << "\n";
      // if(Interesting[i].second){
      // 	Interesting[i].second->print(llvm::outs());
      // 	llvm::outs() << "\n";
      // }
    }

    //errs() << "========B*B===========\n";
    return true;
  }
  
  void AllocFreeInstrument::getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DataLayoutPass>();
      //AU.addRequired<TargetLibraryInfoPass>();
  }
}
  
char MemInstrument::AllocFreeInstrument::ID = 0;

// static void registerMyPass(const PassManagerBuilder &,
//                            PassManagerBase &PM) {
//   PM.add(new AllocFreeInstrument());
// }
// static RegisterStandardPasses
// RegisterMyPass(PassManagerBuilder::EP_EnabledOnOptLevel0,
// 	       registerMyPass);

// static RegisterPass<AllocFreeInstrument>
// X("logAllocDealloc", "Instrument memory allocations and deallocations");
// }
