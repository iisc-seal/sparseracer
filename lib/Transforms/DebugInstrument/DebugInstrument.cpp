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
#include "llvm/Target/TargetMachine.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IntrinsicInst.h"

#include <stdint.h>
using namespace llvm;

namespace {

  struct DebugInstument : public ModulePass { // {{{1
    static char ID; // Pass identification, replacement for typeid
    Constant *MopFn;
    DenseMap<MDNode*, unsigned> _mdnMap; //Map for MDNodes.
    unsigned int _mdnNext = 0;
    typedef DenseMap<MDNode*, unsigned>::iterator mdn_iterator;
    DebugInstument() : ModulePass(ID) {}

    virtual bool runOnModule(Module &M) {
      LLVMContext &Context = M.getContext();
      PointerType *UIntPtr = Type::getInt32PtrTy(Context);
      Type *Void = Type::getVoidTy(Context);
      MopFn = M.getOrInsertFunction("_Z3mopiii", Void,
                                    UIntPtr, Type::getInt32Ty(Context),Type::getInt32Ty(Context),
                                    (Type*)0);
      Function *tmp = cast<Function>(MopFn);
      tmp->setCallingConv(CallingConv::C);

      

      for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
        if (F->isDeclaration()) continue;
	getAllMDNFunc(*F);
        for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
          DebugInstument::runOnBasicBlock(BB, M);
        }
	for(mdn_iterator DMI=_mdnMap.begin();
	    DMI!=_mdnMap.end(); DMI++){
	  
	  errs() << DMI->first->getName() << " " << DMI->second;
	  
	  

	}  
      }
      return true;
    }
    
    void getAllMDNFunc(Function& F){
      SmallVector<std::pair<unsigned, MDNode*>, 4> MDForInst;
      for(Function::iterator BB = F.begin(),
	    E = F.end(); BB!=E; ++BB){
 
        for(BasicBlock::iterator I = BB->begin(),
	      E = BB->end(); I != E; ++I){
	  //get the Metadata declared in the llvm intrinsic functions such as llvm.dbg.declare()
	  if(CallInst* CI = dyn_cast<CallInst>(I)){
	    if(Function *F = CI->getCalledFunction()){
	      if(F->getName().startswith("llvm.")){
		for(unsigned i = 0, e = I->getNumOperands(); i!=e; ++i){
		  if(MDNode *N = dyn_cast_or_null<MDNode>(I->getOperand(i))){
		    createMetadataSlot(N);
		  }
		}
	      }
	    }
	  }
 
	  //Get all the mdnodes attached to each instruction
	  I->getAllMetadata(MDForInst);
	  for(unsigned i = 0, e = MDForInst.size(); i!=e; ++i){
	    createMetadataSlot(MDForInst[i].second);
	  }
	  MDForInst.clear();
	}
      }
    }//end getAllMDNFunc()
 
    void createMetadataSlot(MDNode *N){
      if(!N->isFunctionLocal()){
        mdn_iterator I = _mdnMap.find(N);
        if(I!=_mdnMap.end()){
	  return;
        }
        //the map also stores the number of each metadata node. It is the same order as in the dumped bc file.
        unsigned DestSlot = _mdnNext++;
        _mdnMap[N] = DestSlot;
      }
 
      for (unsigned i = 0, e = N->getNumOperands(); i!=e; ++i){
        if(MDNode *Op = dyn_cast_or_null<MDNode>(N->getOperand(i))){
	  createMetadataSlot(Op);
        }
      }
    }


    void Instrument(BasicBlock::iterator &B, Module &M) {
      
    }

    virtual bool runOnBasicBlock(Function::iterator &BB, Module &M) {
      errs() << "========BB===========\n";
      for (BasicBlock::iterator BI = BB->begin(), BE = BB->end();
           BI != BE; ++BI) { 
        
        errs() << "BI: " << BI->getOpcodeName() << "\n";

	// if (const DbgDeclareInst *DDI = dyn_cast<DbgDeclareInst>(BI)) {
	//   DIVariable *DV = dyn_cast<DIVariable>(DDI->getVariable());
	//   StringRef VarName = DV->getName();
	//   errs() << VarName << "\n";
	// }

	if (MDNode *N = BI->getMetadata("dbg")) {  // Here I is an LLVM instruction
	  DILocation Loc(N);                      // DILocation is in DebugInfo.h
	  unsigned Line = Loc.getLineNumber();
	  StringRef File = Loc.getFilename();
	  StringRef Dir = Loc.getDirectory();
	  errs() << Line << " " << File << " " << Dir << "\n";
	}
	SmallVector<std::pair<unsigned, MDNode*>, 4> MDForInst;
	BI->getAllMetadata(MDForInst);
	SmallVector<StringRef, 8> Names;
	M.getMDKindNames(Names);

	for(SmallVector<std::pair<unsigned, MDNode*>, 4>::iterator
	      II = MDForInst.begin(), EE = MDForInst.end(); II !=EE; ++II) {
	  errs() << "name: " << Names[II->first] << "\n";
	}


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

char DebugInstument::ID = 0;

static RegisterPass<DebugInstument>
X("debugInstr", "Debug Instrument");

