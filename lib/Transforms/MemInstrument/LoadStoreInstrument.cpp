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
    std::string mopName("_Z13mopInstrumentiiPcS_");
    MopFn = M.getOrInsertFunction("mopInstrument", Void,
				  IntptrTy, Type::getInt64Ty(*Context),
				  SBP, SBP,
				  (Type*)0);
    
    Function *tmp = cast<Function>(MopFn);
    tmp->setCallingConv(CallingConv::C);
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
      for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
	LoadStoreInstrument::runOnBasicBlock(BB);
      }
    }
    return true;
  }

  void LoadStoreInstrument::Instrument(BasicBlock::iterator &BI, bool isStore) {
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
    
      // We don't care about addresses on the stack
    if(dyn_cast<AllocaInst>(Addr))
      return;

    Type *OrigPtrTy = Addr->getType();
    Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
    
    // Get size of type being freed
    assert(OrigTy->isSized());
    uint32_t TypeSize = DL->getTypeStoreSizeInBits(OrigTy);
    //errs() << "\n"<< "Type Size is: " << TypeSize << "\n";
      
    Value *Size =  ConstantInt::get(Type::getInt64Ty(Context), TypeSize/8);
    assert((TypeSize % 8) == 0);
      
    //Create a string representing the field being written to
    std::string fieldName = getFieldName(Addr);
    
    //Create a string representing the underlying object being written to
    Value *UnderlyingObject = GetUnderlyingObject(Addr, DL, 0);
    Type *UnderlyingObjectType = UnderlyingObject->getType();

    std::string underlying = getTypeAsString(UnderlyingObjectType);
    std::string original = getTypeAsString(OrigPtrTy);

    bool flag = false;
      
    std::set<std::string>::iterator it;
    for (it = InstrumentedTypes.begin(); it != InstrumentedTypes.end(); ++it){
      std::string current = *it; 
      std::size_t found = underlying.find(current);
      if (found==std::string::npos){
	continue;
      }
      else{
	flag = true;
	break;
      }
    }
    
    if(false == flag)
      return;
    
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
    
    IRB.CreateCall4(MopFn, AddrLong, Size, TypeStringPtr, DebugStringPtr);
  }
  
  bool LoadStoreInstrument::runOnBasicBlock(Function::iterator &BB) {
    //errs() << "========BB===========\n";
    for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
	 BI != BE; ++BI) { 
      if (isa<LoadInst>(BI)) {
	//errs() << "<";
	// Instrument LOAD here
	LoadStoreInstrument::Instrument(BI, false);
      } else {
	if (isa<StoreInst>(BI)) {
	  //errs() << ">";
	  // Instrument STORE here
	  LoadStoreInstrument::Instrument(BI, true);
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

