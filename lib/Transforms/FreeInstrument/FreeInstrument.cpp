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

#define DEBUG_TYPE "mops"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/Pass.h"
//#include "llvm/Target/TargetData.h"
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
  // TsanOfflineInstrument - The second implementation with getAnalysisUsage implemented.
  struct FreeInstrument : public ModulePass { // {{{1
    static char ID; // Pass identification, replacement for typeid
    Constant *MopFn;
    FreeInstrument() : ModulePass(ID) {}

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
    
    MopFn = M.getOrInsertFunction("_Z3mopiPci", Void,
                                  IntptrTy, SBP, Type::getInt32Ty(*Context),
                                  (Type*)0);
    Function *tmp = cast<Function>(MopFn);
    tmp->setCallingConv(CallingConv::C);
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      if (F->isDeclaration()) continue;
        for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
          FreeInstrument::runOnBasicBlock(BB);
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

  void Instrument(BasicBlock::iterator &BI) {
    std::vector<Value*> Args(3);
    llvm::Instruction &IN = *BI;
    errs() << "Dumping Instruction: ";
    IN.dump();

    Value* Addr = IN.getOperand(0)->stripPointerCasts();
    errs() << "Dumping Freed Type: ";
    Addr->stripPointerCasts()->getType()->dump();
    errs()<<"\n";
    
    IRBuilder<> IRB(BI);
    Type *OrigPtrTy = Addr->getType();
    errs() << "Dumping original pointer type: " << "\n";
    OrigPtrTy->dump();
    errs() << "\n";

    Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
    errs() << "Dumping original type: " << "\n";
    OrigTy->dump();
    errs() << "\n";

    assert(OrigTy->isSized());
    uint32_t TypeSize = DL->getTypeStoreSizeInBits(OrigTy);
    errs() << "\n"<< "Type Size is: " << TypeSize << "\n";

    Value *Size =  ConstantInt::get(Type::getInt32Ty(*Context), TypeSize/8);

    assert((TypeSize % 8) == 0);
    Value *AddrLong = IRB.CreatePointerCast(Addr, IntptrTy);
    Value *TypeString = IRB.CreateGlobalString(getTypeAsString(OrigPtrTy));
    Value *StringPtr = IRB.CreateBitCast(TypeString, IRB.getInt8PtrTy());
    IRB.CreateCall3(MopFn, AddrLong, StringPtr, Size);

      // Args[2] = ConstantInt::get(Type::getInt32Ty(BI->getContext()), isStore); 
      // if (Args[0]->getType() == Type::getInt32PtrTy(Context)) {
      // } else {
      //   Args[0] = BitCastInst::CreatePointerCast(Args[0], Type::getInt32PtrTy(BI->getContext()), "", BI);
      // }
      
      // int size = 32;
      // DataLayoutPass *DLP = getAnalysisIfAvailable<DataLayoutPass>();
      // if (!DLP)
      // 	return;
      // const DataLayout *DL = &DLP->getDataLayout();

      // if (Args[0]->getType()->isSized()) {
      //   size = DL->getTypeStoreSizeInBits(Args[0]->getType());
      // }
      // Args[1] = ConstantInt::get(Type::getInt32Ty(BI->getContext()), size/8); 

      // errs() << "Dumping Args[0]: ";
      // Args[0]->dump();
      // errs() << "Dumping Args[1]: ";
      // Args[1]->dump();
      // errs() << "Dumping Args[2]: ";
      // Args[2]->dump();
      // Value *V = CallInst::Create(MopFn, Args, "", BI);
      // errs() << "Dumping CallInst: ";
      // V->dump();
    }

  virtual bool runOnBasicBlock(Function::iterator &BB) {
    errs() << "========BB===========\n";
    std::string freeName("free");
    for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
         BI != BE; ++BI) { 
      if (CallInst * CI = dyn_cast<CallInst>(BI)) {
	if (Function * CalledFunc = CI->getCalledFunction()) {
	  std::string name = CalledFunc->getName();
	  if(freeName.compare(name) == 0){
	    FreeInstrument::Instrument(BI);
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
  
char FreeInstrument::ID = 0;

static RegisterPass<FreeInstrument>
X("freeinstr", "Instrument Frees");

