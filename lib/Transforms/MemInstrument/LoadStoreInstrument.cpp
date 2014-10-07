//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include "LoadStoreInstrument.h"
#include "AllocFreeInstrument.h"

using namespace llvm;
namespace MemInstrument {

  bool LoadStoreInstrument::runOnModule(Module &M) {
    DataLayoutPass *DLP = getAnalysisIfAvailable<DataLayoutPass>();
    if (!DLP)
      return false;
    DL = &DLP->getDataLayout();
    
    Context = &(M.getContext());
    int LongSize = DL->getPointerSizeInBits();
    IntptrTy = Type::getIntNTy(*Context, LongSize);
    
    Type *Void = Type::getVoidTy(*Context);
    // MopFn = M.getOrInsertFunction("_Z3mopiii", Void,
    //                               UIntPtr, Type::getInt32Ty(*Context),Type::getInt32Ty(*Context),
    //                               (Type*)0);
    const Type *SBP = Type::getInt8PtrTy(*Context);
    std::string mopName("_Z13mopInstrumentiiPcS_S_");
    MopFn = M.getOrInsertFunction("mopInstrument", Void,
				  IntptrTy, Type::getInt64Ty(*Context),
				  SBP, SBP, SBP,
				  (Type*)0);
    
    Function *tmp = cast<Function>(MopFn);
    tmp->setCallingConv(CallingConv::C);
    
    std::map<std::string, std::string> funcNameToDirName = getDebugInformation(M);

    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      //if (F->isDeclaration()) continue;
      std::string fName = F->getName().str();
      
      std::string dirName = "";

      // Try to abort early based on the directories to be instrumented
      std::map<std::string,std::string>::const_iterator search = funcNameToDirName.find(fName);
      
      // if(search != funcNameToDirName.end()) {
      // 	dirName = search->second;
      // }
      
      //  if(dirName.compare("")!=0)
      // 	 if(!shouldInstrumentDirectory(dirName))
      // 	   continue;

      // 	   || dirName.find("nsprpub/pr/src") != std::string::npos
      // 	   || dirName.find("ipc/chromium/src/base") != std::string::npos)
      // 	  continue;

      // Debug output to see that the IR being generated is OK	   
      // llvm::outs() << "Processing " << fName+" in "+dirName << "\n";
      // if(fName.find("Capture") != std::string::npos){
      // 	llvm::outs() << "Instrumenting " << fName << "\n";
      // 	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
      // 	  llvm::outs() << *I << "\n";
      // 	for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
      // 	    LoadStoreInstrument::runOnBasicBlock(BB, fName, dirName);
      // 	}
      // 	llvm::outs() << "After: \n";
      // 	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
      // 	  llvm::outs() << *I << "\n";
      // 	continue;
      // }
      // else
      // 	continue;

      // don't instrument functions that have to deal with memory management
      // and our own instrumentation routines, if they show up somehow
      const bool found = (blacklist.find(fName) != blacklist.end());
      if(found)
	continue;
      // don't instrument syslog
      if(fName.find("syslog")!=std::string::npos)
	continue;
	// don't instrument functions used in mopInstrument
        // or functions that are called transitively
      // if(fName.find("PR_GetThreadID")!=std::string::npos || 
      // 	 fName.find("PR_GetCurrentThread")!=std::string::npos || 
      // 	 fName.find("_PR_") != std::string::npos ||
      // 	 fName.find("getID") != std::string::npos ||
      // 	 fName.find("pt_AttachThread") != std::string::npos) 
      // 	continue;
      
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
	LoadStoreInstrument::runOnBasicBlock(BB, fName, dirName);
      }
    }
    return true;
  }

  void LoadStoreInstrument::Instrument(BasicBlock::iterator &BI, bool isStore, std::string fName) {
    std::vector<Value*> Args(3);
    
    llvm::Instruction &IN = *BI;
    LLVMContext &Context = BI->getContext();
    // errs() << "Dumping Instruction: ";
    // IN.dump();
    
    // Get Address being accessed
    Value *Addr;
    if(isStore)
      Addr = (static_cast<StoreInst&>(IN).getPointerOperand());
    else
      Addr = (static_cast<LoadInst&>(IN).getPointerOperand());
    
    // We don't care about pointer operands that aren't double pointers
    // Edit: Actually, we do :( 
    // if(!isPointerToPointer(Addr))
    //   return;

    // This is too simplistic :( The problem is, heap addresses can be stored
    // into stack variable, say as a return from a function call
    //if(dyn_cast<AllocaInst>(Addr))
    //return;
    
    Type *OrigPtrTy = Addr->getType();
    Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
    
    // Get size of type being accessed
    assert(OrigTy->isSized());
    uint32_t TypeSize = DL->getTypeStoreSizeInBits(OrigTy);
    // errs() << "\n"<< "Got here: " << TypeSize << "\n";
      
    Value *Size =  ConstantInt::get(Type::getInt64Ty(Context), TypeSize/8);
    assert((TypeSize % 8) == 0);
      
    //Create a string representing the field being written to
    std::string fieldName = getFieldName(Addr);
    
    //Create a string representing the underlying object being written to
    Value *UnderlyingObject = GetUnderlyingObject(Addr, DL, 0);
    Type *UnderlyingObjectType = UnderlyingObject->getType();

    std::string underlying = getTypeAsString(UnderlyingObjectType);
    std::string original = getTypeAsString(OrigPtrTy);

    // bool flag = false;
      
    // std::set<std::string>::iterator it;
    // for (it = InstrumentedTypes.begin(); it != InstrumentedTypes.end(); ++it){
    //   std::string current = *it; 
    //   std::size_t found = underlying.find(current);
    //   if (found==std::string::npos){
    // 	continue;
    //   }
    //   else{
    // 	flag = true;
    // 	break;
    //   }
    // }
    
    // if(false == flag)
    //   return;
    
    //Start writing out bitcode instructions to perform the instrumentation
    IRBuilder<> IRB(BI);
    //Cast the address being written to into an int   
    Value *AddrLong = IRB.CreatePointerCast(Addr, IntptrTy);
    
    //Create a string representing the type being written to  
    Value *TypeString = IRB.CreateGlobalString(original+"("+underlying+","+ fieldName +")");
    
    //Get a pointer to the string
    Value *TypeStringPtr = IRB.CreateBitCast(TypeString, IRB.getInt8PtrTy());
    
    //Get a string representing source line number information
    std::string tag = "load";
    if(isStore)
      tag = "store";
    Value *DebugLocationString = IRB.CreateGlobalString(getSourceInfoAsString(BI, tag));
    
    //Get a pointer to the string
    Value *DebugStringPtr = IRB.CreateBitCast(DebugLocationString, IRB.getInt8PtrTy());
 
    Value *FunctionNameString = IRB.CreateGlobalString(fName);
    
    //Get a pointer to the string
    Value *FunctionStringPtr = IRB.CreateBitCast(FunctionNameString, IRB.getInt8PtrTy());
     
    IRB.CreateCall5(MopFn, AddrLong, Size, TypeStringPtr, DebugStringPtr, FunctionStringPtr);
  }
  
  bool LoadStoreInstrument::runOnBasicBlock(Function::iterator &BB, 
					    std::string fName, std::string dName) {
    //errs() << "========BB===========\n";
    for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
	 BI != BE; ++BI) { 
      // Getting the dirname earlier failed. Try again using the instruction

      

      if(dName.compare("")==0)
	if(!shouldInstrumentDirectory(getDirName(BI)))  //&&
	  // fName.find("assign_assuming_AddRef") == std::string::npos)
	 //	  && fName.find("CheckAcquire") == std::string::npos )
	  //!shouldInstrumentFile(getFileName(BI)))
	 continue;
      
      // llvm::outs() << "Processing " << fName << " in " << getFileName(BI) << " \n";
      // llvm::outs() << "shouldInstrumentFile " << shouldInstrumentFile(getFileName(BI)) << " \n";
      if (isa<LoadInst>(BI)) {
	//errs() << "<";
	// Instrument LOAD here
	//llvm::outs() << "\n Load: " << getDirName(BI) << "\n";
	LoadStoreInstrument::Instrument(BI, false, fName);
      } else {
	if (isa<StoreInst>(BI)) {
	  //errs() << ">";
	  // Instrument STORE here
	  //llvm::outs() << "\n Store: " << getDirName(BI) << "\n";
	  LoadStoreInstrument::Instrument(BI, true, fName);
	} else {
	  //errs() << " ";
	}
      }
      //errs() << "BI: " << BI->getOpcodeName() << "\n";
    }
    //errs() << "========BB===========\n";
    return true;
  }

  void LoadStoreInstrument::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<DataLayoutPass>();
  }
}
// }}}

char MemInstrument::LoadStoreInstrument::ID = 0;
// // char TsanOnlineInstrument::ID = 0;

// static void registerMyPass(const PassManagerBuilder &,
//                            PassManagerBase &PM) {
//   PM.add(new LoadStoreInstrument());
// }

// RegisterStandardPasses
// RegisterMyPass(PassManagerBuilder::EP_EnabledOnOptLevel0,
//                registerMyPass);

// static RegisterPass<LoadStoreInstrument>
// X("loadstoreinstr", "Load Store Instrumentation");

