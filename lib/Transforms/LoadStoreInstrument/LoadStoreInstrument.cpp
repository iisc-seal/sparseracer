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
#include "llvm/PassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/LLVMContext.h"
//#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/ValueTracking.h"

#include <stdint.h>
using namespace llvm;


namespace {
  // TsanOfflineInstrument - The second implementation with getAnalysisUsage implemented.
  struct LoadStoreInstrument : public ModulePass { // {{{1
    static char ID; // Pass identification, replacement for typeid
    Constant *MopFn;
    Type *IntptrTy;
    LLVMContext *Context;
    const DataLayout *DL;


    LoadStoreInstrument() : ModulePass(ID) {}

    std::string getTypeAsString(Type* T){
      std::string type_str;
      llvm::raw_string_ostream rso(type_str);
      T->print(rso);
      return rso.str();
    }

    std::string getFieldName(Value *Addr){
      if (GEPOperator *GEP = dyn_cast<GEPOperator>(Addr)) 
	return getTypeAsString(GEP->getPointerOperand()->getType());
      
      return "";
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


    virtual bool runOnModule(Module &M) {
      
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
      MopFn = M.getOrInsertFunction("_Z13mopInstrumentiiPcS_", Void,
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

    void Instrument(BasicBlock::iterator &BI, bool isStore) {
      std::vector<Value*> Args(3);

      llvm::Instruction &IN = *BI;
      LLVMContext &Context = BI->getContext();
      errs() << "Dumping Instruction: ";
      IN.dump();

      IRBuilder<> IRB(BI);

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
      errs() << "\n"<< "Type Size is: " << TypeSize << "\n";
      
      Value *Size =  ConstantInt::get(Type::getInt64Ty(Context), TypeSize/8);
      assert((TypeSize % 8) == 0);

      //Cast the address being written to into an int   
      Value *AddrLong = IRB.CreatePointerCast(Addr, IntptrTy);
      
      //Create a string representing the field being written to
      std::string fieldName = getFieldName(Addr);
      
      //Create a string representing the underlying object being written to
      Value *UnderlyingObject = GetUnderlyingObject(Addr, DL, 0);
      Type *UnderlyingObjectType = UnderlyingObject->getType();

      std::string underlying = getTypeAsString(UnderlyingObjectType);
      std::string original = getTypeAsString(OrigPtrTy);

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

      // if (isStore) {
      //   Args[0] = (static_cast<StoreInst&>(IN).getPointerOperand());
      // 	errs() << "Dumping Store Pointer Operand Type: ";
      // 	const Value *BV = GetUnderlyingObject(Args[0], DL, 0);
      // 	BV->getType()->dump();
      // 	errs()<<"\n";
      // } else {
      //   Args[0] = (static_cast<LoadInst&>(IN).getPointerOperand());
      // 	errs() << "Dumping Pointer Operand Type: ";
      // 	Args[0]->stripPointerCasts()->getType()->dump();
      // 	errs()<<"\n";
      // }
      
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
      for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
           BI != BE; ++BI) { 
        if (isa<LoadInst>(BI)) {
          errs() << "<";
          // Instrument LOAD here
          LoadStoreInstrument::Instrument(BI, false);
        } else {
          if (isa<StoreInst>(BI)) {
            errs() << ">";
            // Instrument STORE here
            LoadStoreInstrument::Instrument(BI, true);
          } else {
            errs() << " ";
          }
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
  // }}}
}

char LoadStoreInstrument::ID = 0;
// char TsanOnlineInstrument::ID = 0;

static void registerMyPass(const PassManagerBuilder &,
                           PassManagerBase &PM) {
  PM.add(new LoadStoreInstrument());
}

RegisterStandardPasses
RegisterMyPass(PassManagerBuilder::EP_EnabledOnOptLevel0,
               registerMyPass);

static RegisterPass<LoadStoreInstrument>
X("loadstoreinstr", "Load Store Instrumentation");

// static RegisterPass<TsanOnlineInstrument>
// Y("online", "TSAN Online");


// INITIALIZE_PASS(TsanOfflineInstrument, "offline",
//                 "Hello World Pass (with getAnalysisUsage implemented)",
//                 false, false);
// INITIALIZE_PASS(TsanOnlineInstrument, "online",
//                 "Hello World Pass (with getAnalysisUsage implemented)",
//                 false, false);
