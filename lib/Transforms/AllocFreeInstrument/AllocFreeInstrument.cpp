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

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"

#include <stdint.h>
using namespace llvm;

Type *IntptrTy;
LLVMContext *Context;
const DataLayout *DL;

namespace {

  struct AllocFreeInstrument : public ModulePass { // {{{1
    static char ID; // Pass identification, replacement for typeid
    Constant *MDallocFn;
    Constant *MAllocFn;
    std::string freeName;
    std::string delName;
    std::string arrayDelName;
    std::string mallocName;
    std::string newName;
    std::string arrayNewName;
    AllocFreeInstrument() : ModulePass(ID) {
      initializeFunctionNames();
    }

    void initializeFunctionNames(){
      freeName = std::string("free");
      delName = std::string("_ZdlPv");
      arrayDelName = std::string("_ZdaPv");
      mallocName = std::string("malloc");
      newName = std::string("_Znwm");
      arrayNewName = std::string("_Znam");
    }

    virtual bool runOnModule(Module &M) {

    DataLayoutPass *DLP = getAnalysisIfAvailable<DataLayoutPass>();
    if (!DLP)
      return false;
    DL = &DLP->getDataLayout();

    Context = &(M.getContext());
    int LongSize = DL->getPointerSizeInBits();
    IntptrTy = Type::getIntNTy(*Context, LongSize);

    Type *Void = Type::getVoidTy(*Context);
    const Type *SBP = Type::getInt8PtrTy(*Context);
    
    MAllocFn = M.getOrInsertFunction("_Z8mopAllociiPcS_", Void,
                                  IntptrTy, Type::getInt64Ty(*Context), 
				  SBP, SBP,
                                  (Type*)0);
    MDallocFn = M.getOrInsertFunction("_Z10mopDeallociiPcS_", Void,
                                  IntptrTy, Type::getInt32Ty(*Context), 
				  SBP, SBP,
                                  (Type*)0);
    (cast<Function>(MAllocFn))->setCallingConv(CallingConv::C);
    (cast<Function>(MDallocFn))->setCallingConv(CallingConv::C);

    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
        for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
          AllocFreeInstrument::runOnBasicBlock(BB);
        }
    }
    return true;
  }

  Value* CastToCStr(Value *V, IRBuilder<> &B) {
      return B.CreateBitCast(V, B.getInt8PtrTy(), "cstr");
  }  

  std::string getTypeAsString(Type* T){
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    T->print(rso);
    return rso.str();
  }

    std::string getSourceInfoAsString(Instruction *I, std::string name){
    if (MDNode *N = I->getMetadata("dbg")) {  // Here I is an LLVM instruction
      DILocation Loc(N);                      // DILocation is in DebugInfo.h
      std::string Line = std::to_string(Loc.getLineNumber());
      std::string File = Loc.getFilename().str();
      std::string Dir = Loc.getDirectory().str();
      return (Dir + "/" + File + ":" + Line + "(" + name + ")");
    }
    return name;
  }  

    void InstrumentDealloc(BasicBlock::iterator &BI, std::string fName) {
    
    // Output instruction currently being processed
    llvm::Instruction &IN = *BI;
    errs() << "Dumping Instruction: ";
    IN.dump();

    // Get Address and being freed
    Value* Addr = IN.getOperand(0)->stripPointerCasts();
    errs() << "Dumping Freed Type: ";
    Addr->stripPointerCasts()->getType()->dump();
    errs()<<"\n";

    // Get Type Information
    IRBuilder<> IRB(BI);
    Type *OrigPtrTy = Addr->getType();
    errs() << "Dumping original pointer type: " << "\n";
    OrigPtrTy->dump();
    errs() << "\n";
    Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
    errs() << "Dumping original type: " << "\n";
    OrigTy->dump();
    errs() << "\n";

    // Get size of type being freed
    assert(OrigTy->isSized());
    uint32_t TypeSize = DL->getTypeStoreSizeInBits(OrigTy);
    errs() << "\n"<< "Type Size is: " << TypeSize << "\n";

    Value *Size =  ConstantInt::get(Type::getInt32Ty(*Context), TypeSize/8);
    assert((TypeSize % 8) == 0);
    
    //Cast the address being written to into an int *
    Value *AddrLong = IRB.CreatePointerCast(Addr, IntptrTy);

    //Create a string representing the type being written to
    Value *TypeString = IRB.CreateGlobalString(getTypeAsString(OrigPtrTy));

    //Get a pointer to the string
    Value *TypeStringPtr = IRB.CreateBitCast(TypeString, IRB.getInt8PtrTy());

    //Get a string representing source line number information
    Value *DebugLocationString = IRB.CreateGlobalString(getSourceInfoAsString(BI, fName));

    //Get a pointer to the string
    Value *DebugStringPtr = IRB.CreateBitCast(DebugLocationString, IRB.getInt8PtrTy());

    //Track if this is an allocate (or deallocate)
    // Value *Allocate = ConstantInt::get(Type::getInt32Ty(*Context), isAlloc);

    IRB.CreateCall4(MDallocFn, AddrLong, Size, TypeStringPtr, DebugStringPtr);
  }

  void InstrumentAlloc(BasicBlock::iterator &BI, std::string fName) {

    Type *OrigTy;
    Instruction* Original = BI;
    ++BI; 
    BitCastInst *Inst;
    if ((Inst = dyn_cast<BitCastInst>(BI))) {
      OrigTy = Inst->getType();
      errs() << "\n";
      OrigTy->dump();
    }
    errs() << "Dumping use: ";
    Inst->dump();
    IRBuilder<> IRB(Inst);
    
    // Get the address allocated
    Value *AddrLong = IRB.CreatePointerCast(Original, IntptrTy);
    
    // Get the number of bytes allocated
    Value *MemSize = dyn_cast<llvm::ConstantInt>(Original->getOperand(0));
    
    //Create a string representing the type being written to
     Value *TypeString = IRB.CreateGlobalString(getTypeAsString(OrigTy));

    //Get a pointer to the string                                                                         
    Value *TypeStringPtr = IRB.CreateBitCast(TypeString, IRB.getInt8PtrTy());

    //Get a string representing source line number information                                            
    Value *DebugLocationString = IRB.CreateGlobalString(getSourceInfoAsString(BI, fName));

    //Get a pointer to the string                                                                         
    Value *DebugStringPtr = IRB.CreateBitCast(DebugLocationString, IRB.getInt8PtrTy());

    IRB.CreateCall4(MAllocFn, AddrLong, MemSize, TypeStringPtr, DebugStringPtr);

    BI--;
    
  }

  virtual bool runOnBasicBlock(Function::iterator &BB) {
    errs() << "========BB===========\n";
    for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
         BI != BE; ++BI) { 
      if (CallInst * CI = dyn_cast<CallInst>(BI)) {
	if (Function * CalledFunc = CI->getCalledFunction()) {
	  std::string name = CalledFunc->getName();
	  
	  if(freeName.compare(name) == 0 || delName.compare(name) == 0
	     || arrayDelName.compare(name) == 0){
	    AllocFreeInstrument::InstrumentDealloc(BI, name);
	  }
	  if(mallocName.compare(name) == 0 || newName.compare(name) == 0
	     || arrayNewName.compare(name) == 0){
	    AllocFreeInstrument::InstrumentAlloc(BI, name);
	  }
	}
      } 
      else {
          errs() << " ";
      }
      
      errs() << "BI: " << BI->getOpcodeName() << "\n";
    }
    errs() << "========BB===========\n";
    return true;
  }
  
  private:
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DataLayoutPass>();
    }
  };
  
}
  
char AllocFreeInstrument::ID = 0;

static RegisterPass<AllocFreeInstrument>
X("logAllocDealloc", "Instrument memory allocations and deallocations");

