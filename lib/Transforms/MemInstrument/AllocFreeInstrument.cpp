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

    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      // don't instrument syslog
      if(F->getName().str().find("syslog")!=std::string::npos)
	continue;
      // if (!shouldInstrument(demangleFunctionName(F->getName().str()), whiteList))
      // 	continue;
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
	std::string name = F->getName().str();
	AllocFreeInstrument::runOnBasicBlock(BB, name);
      }
    }
    return true;
  }
    

  void AllocFreeInstrument::InstrumentDealloc(BasicBlock::iterator &BI, std::string fName) {
    
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
      errs() << "Failed to track free on :"; 
      OrigTy->dump();
      return;
    }
    assert(OrigTy->isSized());
    uint32_t TypeSize = DL->getTypeStoreSizeInBits(OrigTy);
    //errs() << "\n"<< "Type Size is: " << TypeSize << "\n";

    Value *Size =  ConstantInt::get(Type::getInt64Ty(*Context), TypeSize/8);
    assert((TypeSize % 8) == 0);
    
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

  void AllocFreeInstrument::InstrumentAlloc(BitCastInst* Succ, CallInst *Original, std::string fName) {

    // This is fragile in the sense that it assumes that a cast
    // instruction always follows an alloc instruction where the
    // address returned by the alloc is cast to the type of the
    // register. The pass will break if the assumption does not hold.

    Type *OrigTy;
    
    // for (Value::use_iterator i = Original->use_begin(), e = Original->use_end(); i != e; ++i)
    //   if (Instruction *Inst = dyn_cast<Instruction>(*i)) {
    // 	errs() << "F is used in instruction:\n";
    // 	errs() << *Inst << "\n";
    //   }


    OrigTy = Succ->getType();
    //errs() << OrigTy << "\n";
    //OrigTy->dump();
    
    
    IRBuilder<> IRB(Succ);
    
    // Get the address allocated
    Value *AddrLong = IRB.CreatePointerCast(Original, IntptrTy);
    //errs() << "Address " << *AddrLong << "\n";
    // Get the number of bytes allocated
    Value *MemSize;
    if(isa<llvm::ConstantInt>(Original->getOperand(0)))
      MemSize = dyn_cast<llvm::ConstantInt>(Original->getOperand(0));
    else
      MemSize = Original->getOperand(0);

    //errs() << "Size " << *MemSize<< "\n";
    //Create a string representing the type being written to
    Value *TypeString = IRB.CreateGlobalString(getTypeAsString(OrigTy));
    //errs() << "Type String" << *TypeString<< "\n";
    //Get a pointer to the string   
    Value *TypeStringPtr = IRB.CreateBitCast(TypeString, IRB.getInt8PtrTy());
    //errs() << "TYS PTr" << *TypeStringPtr<< "\n";
    //Get a string representing source line number information
    Value *DebugLocationString = IRB.CreateGlobalString(getSourceInfoAsString(Original, fName));
    //errs() << "DebugLocPtr" << *DebugLocationString<< "\n";
    //Get a pointer to the string
    Value *DebugStringPtr = IRB.CreateBitCast(DebugLocationString, IRB.getInt8PtrTy());
    //errs() << "DebugStrPtr" << *DebugStringPtr<< "\n";
    //errs() <<"Alloc! \n";
    //Get a string representing the function this write is in
    Value *FunctionNameString = IRB.CreateGlobalString(fName);
    
    //Get a pointer to the string representing the function name
    Value *FunctionStringPtr = IRB.CreateBitCast(FunctionNameString, IRB.getInt8PtrTy());


    IRB.CreateCall5(MAllocFn, AddrLong, MemSize, TypeStringPtr, DebugStringPtr, FunctionStringPtr);
    //errs() << "Problem! \n";
    //errs() << *CI << "\n";
        
  }

  bool AllocFreeInstrument::runOnBasicBlock(Function::iterator &BB, std::string callerName) {
    //errs() << "========BB===========\n";
    bool flag = false;
    for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
         BI != BE; ++BI) { 
      
      if(flag){
	//BI->dump();
      	//errs() << "Got here! \n";
	flag = false;
      }
      
      if (CallInst * CI = dyn_cast<CallInst>(BI)) {
	std::string dirName = getDirName(CI);
        // llvm::outs() << dirName << "\n";
	if(!shouldInstrumentDirectory(dirName))
	  continue;
	if (Function * CalledFunc = CI->getCalledFunction()) {
	  std::string name = CalledFunc->getName();
	  // llvm::outs() << name << "\n"; 
	  const bool found = (freeFunctions.find(name) != freeFunctions.end());
	  if(found){
	    // delete and delete[] are overloaded to redirect to moz_free
	    // so avoid instrumenting twice
	    if(callerName.compare("_ZdlPv") == 0 || callerName.compare("_ZdaPv") == 0)
	      continue;
	    AllocFreeInstrument::InstrumentDealloc(BI, callerName);
	  }
	  
	}
      }
      // assume an alloc is always followed by a bitcast
      else if (BitCastInst *BCI = dyn_cast<BitCastInst>(BI)) {
	if (CallInst * CI = dyn_cast<CallInst>(BCI->getOperand(0))) {
	  if (Function * CalledFunc = CI->getCalledFunction()) {
	    std::string name = CalledFunc->getName();
	    const bool found = (allocFunctions.find(name) != allocFunctions.end());
	    if(found){
	      //BI->dump();
	      AllocFreeInstrument::InstrumentAlloc(BCI, CI, callerName);
	      //errs() << "Alloc insert success!" ;
	      flag = true;
	      //BI->dump();
	    }
	  }
	}
      }
      else {
	//errs() << " ";
      }
      
      //errs() << "BI: " << BI->getOpcodeName() << "\n";
    }
    //errs() << "========BB===========\n";
    return true;
  }
  
  void AllocFreeInstrument::getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DataLayoutPass>();
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
